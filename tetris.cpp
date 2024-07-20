#include "tetris.hpp"
#include "rayui.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <raylib.h>
#include <string>
#include <utility>

void Game::saveTetromino() { tetromino->saveState(); }

std::vector<std::vector<Color>> Game::palette = {
    // Palette 1: Cool Blues
    {SKYBLUE, DARKBLUE, LIGHTGRAY, BLUE, WHITE},
    // Palette 2: Warm Tones
    {YELLOW, GOLD, ORANGE, BEIGE, RAYWHITE},
    // Palette 3: Greens
    {GREEN, LIME, DARKGREEN, LIGHTGRAY, RAYWHITE},
    // Palette 4: Purples
    {PURPLE, VIOLET, DARKPURPLE, LIGHTGRAY, WHITE},
    // Palette 5: Reds
    {RED, MAROON, PINK, BEIGE, RAYWHITE},
    // Palette 6: Earth Tones
    {BROWN, DARKBROWN, BEIGE, GOLD, RAYWHITE},
};
std::unordered_map<Shape, std::vector<Vec2>> Game::shapePatterns = {
    {Shape::L, {{-1, 1}, {-1, 0}, {0, 0}, {1, 0}}},
    {Shape::J, {{-1, 0}, {0, 0}, {1, 0}, {1, 1}}},
    {Shape::Z, {{-1, 0}, {0, 0}, {0, 1}, {1, 1}}},
    {Shape::S, {{-1, 1}, {0, 1}, {0, 0}, {1, 0}}},
    {Shape::I, {{-1, 0}, {0, 0}, {1, 0}, {2, 0}}},
    {Shape::T, {{-1, 0}, {0, 0}, {1, 0}, {0, 1}}},
    {Shape::O, {{0, 0}, {0, 1}, {1, 0}, {1, 1}}},
};

Game::Game() {
  board = Board();
  setNextShapeAndColor();
  blockTexture = LoadTexture("res/block.png");
  blockTxSourceRect =
      Rectangle{0, 0, (float)blockTexture.width, (float)blockTexture.height};
  gameGrid = createGrid();
  inMenu = true;
  scoreFile.read();
  generateGravityLevels(100);
}

void Game::processGameLogic() {

  // if an animation is active, we pause the game.
  if (!animation_queue.empty()) {
    return;
  }

  if (!tetromino) {
    // spawn a new tetromino, cause the last one landed.
    auto shape = nextShape;
    auto color = nextColor;
    tetromino = std::make_unique<Tetromino>(shape, color);
    setNextShapeAndColor();

    tetromino->saveState();

    // game-over condition. currently, this is premature sometimes.
    if (resolveCollision(tetromino)) {
      if (score > scoreFile.high_score) {
        scoreFile.high_score = score;
      }
      inMenu = true;
      return;
    }
  }
  
  if (IsKeyPressed(KEY_F)) {
    for (int i = 16; i < 20; ++i) {
      for (auto &cell: board.rows[i]) {
        cell.empty = false;        
      }
    }
    auto linesToClear = checkLines();
    auto linesCleared = clearLines(linesToClear);
    return;
  }
  

  // used to increment until >= 1 so we can have sub-frame velocity for the
  // tetromino.
  static float budge = 0.0;

  cleanTetromino(tetromino);

  auto horizontal = delayedAutoShift();

  auto executeMovement = [&](std::function<void()> fun) -> bool {
    tetromino->saveState();
    fun();
    return resolveCollision(tetromino);
  };

  bool turnLeft = IsKeyPressed(KEY_Z);
  bool turnRight = IsKeyPressed(KEY_X) || IsKeyPressed(KEY_UP);

  bool moveDown = IsKeyDown(KEY_DOWN);

  if (auto gpad = FindGamepad(); gpad != -1) {
    // z or A (xbox controller)
    turnLeft = turnLeft ||
               IsGamepadButtonPressed(gpad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    // x or B (xbox controller)
    turnRight = turnRight ||
                IsGamepadButtonPressed(gpad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
    // down arrow or dpad down
    moveDown =
        moveDown || IsGamepadButtonPressed(gpad, GAMEPAD_BUTTON_LEFT_FACE_DOWN);
  }

  if (turnLeft) {
    executeMovement([&]() { tetromino->spinLeft(); });
  }

  if (turnRight) {
    executeMovement([&] { tetromino->spinRight(); });
  }

  if (horizontal.left) {
    executeMovement([&] { tetromino->position.x--; });
  }
  if (horizontal.right) {
    executeMovement([&] { tetromino->position.x++; });
  }

  auto oldGravity = gravity;
  if (moveDown && gravity <= 0.5f) {
    gravity = 0.5;
  }

  // check if this piece hit the floor, or another piece.
  auto landed = executeMovement([&] {
    budge += gravity;
    auto floored = std::floor(budge);
    if (floored > 0) {
      tetromino->position.y += 1;
      budge = 0;
    }
  });

  for (const auto &idx : getIndices(tetromino)) {
    if (idx.x < 0 || idx.x >= boardWidth || idx.y < 0 || idx.y >= boardHeight) {
      continue;
    }
    auto &cell = board.get_cell(idx.x, idx.y);
    cell.empty = false;
    cell.color = tetromino->color;
  }

  // if we landed, we leave the cells where they are and spawn a new piece.
  // also check for line clears and tetrises.
  if (landed) {
    tetromino.reset(nullptr);
    
    auto linesToClear = checkLines();
    auto linesCleared = clearLines(linesToClear);
    adjustScoreAndLevel(linesCleared);
  }

  gravity = oldGravity;
}

void Game::setNextShapeAndColor() {
  static int num_shapes = (int)Shape::O + 1;
  auto shape = Shape(rand() % num_shapes);
  nextShape = shape;
  nextColor = (size_t)((int)shape % (int)palette[paletteIdx].size());
}

void Game::cleanTetromino(std::unique_ptr<Tetromino> &tetromino) {
  auto indices = getIndices(tetromino);
  for (const auto &idx : indices) {
    if (idx.y < 0 || idx.y >= boardHeight || idx.x < 0 || idx.x >= boardWidth) {
      continue;
    }
    auto &cell = board.get_cell(idx.x, idx.y);
    cell.empty = true;
    cell.color = 0;
  }
}

std::vector<size_t> Game::checkLines() {
  std::vector<size_t> linesToBurn = {};
  size_t i = 0;
  for (const auto &row : board.rows) {
    bool full = true;
    for (const auto &cell : row) {
      if (cell.empty) {
        full = false;
        break;
      }
    }
    if (full) {
      linesToBurn.push_back(i);
    }
    ++i;
  }

  return linesToBurn;
}

Grid Game::createGrid() {
  Grid grid({26, 20});
  grid.style.background = BG_COLOR;

  auto linesLabel =
      grid.emplace_element<DynamicLabel>(Position{1, 1}, Size{7, 1});
  linesLabel->text = "Lines:";
  grid.emplace_element<NumberText>(Position{1, 2}, Size{7, 1},
                                   &totalLinesCleared, WHITE);

  auto playfield = createBoardGrid();
  playfield->position = {8, 0};
  playfield->size = {10, 20};
  grid.elements.push_back(playfield);

  int yPos = 1, height = 1;

  auto topLabel =
      grid.emplace_element<DynamicLabel>(Position{19, yPos}, Size{7, height});
  yPos += height;
  topLabel->text = "Top:";
  grid.emplace_element<NumberText>(Position{19, yPos}, Size{7, height},
                                   &scoreFile.high_score, WHITE);
  yPos += height;
  yPos += 1;

  auto scoreLabel =
      grid.emplace_element<DynamicLabel>(Position{19, yPos}, Size{7, height});
  yPos += height;
  scoreLabel->text = "Score:";
  grid.emplace_element<NumberText>(Position{19, yPos}, Size{7, height}, &score,
                                   WHITE);
  yPos += height;
  yPos += 1;

  auto nextPieceLabel =
      grid.emplace_element<DynamicLabel>(Position{19, yPos}, Size{7, height});
  yPos += height;
  nextPieceLabel->text = "Next:";
  height = 4;
  auto pieceViewer = grid.emplace_element<PieceViewer>(Position{19, yPos},
                                                       Size{4, height}, *this);
  pieceViewer->style.background = BLACK;
  yPos += height;
  yPos += 1;

  height = 1;
  auto levelLabel =
      grid.emplace_element<DynamicLabel>(Position{19, yPos}, Size{7, height});
  yPos += height;
  levelLabel->text = "Level:";
  grid.emplace_element<NumberText>(Position{19, yPos}, Size{7, height}, &level,
                                   WHITE);
  yPos += height;
  yPos += 1;

  auto mainMenuButton = grid.emplace_element<Button>(
      Position{19, yPos}, Size{5, height}, "Main Menu",
      std::function<void()>([&]() { inMenu = true; }));
  mainMenuButton->fontSize = 24;
  yPos += height;

  auto resetButton =
      grid.emplace_element<Button>(Position{19, yPos}, Size{5, height}, "Reset",
                                   std::function<void()>([&]() { reset(); }));
  resetButton->fontSize = 24;
  yPos += height;

  return grid;
}

bool Game::resolveCollision(std::unique_ptr<Tetromino> &tetromino) {
  for (const auto idx : getIndices(tetromino)) {
    if (
      idx.y >= boardHeight ||
      idx.x < 0 ||
      idx.x >= boardWidth ||
      board.collides(idx)
    ) {
      tetromino->position = tetromino->prev_position;
      tetromino->orientation = tetromino->prev_orientation;
      return true;
    }
  }
  return false;
}

bool Board::collides(Vec2 pos) noexcept {
  int x = pos.x;
  int y = pos.y;
  return
    y < (int)rows.size() &&
    y >= 0 &&
    x < (int)rows[y].size() &&
    x >= 0 &&
    !get_cell(x, y).empty;
}

Game::~Game() { UnloadTexture(blockTexture); }

void Game::reset() {
  score = 0;
  gravity = gravityLevels[level];
  linesClearedThisLevel = 0;
  totalLinesCleared = 0;
  board = {}; // reset the grid state.
  tetromino = nullptr;
  setNextShapeAndColor();
  gameGrid = createGrid();
}

HorizontalInput Game::delayedAutoShift() {
  static float dasDelay = 0.2f;
  static float arrDelay = 0.05f;
  static float dasTimer = 0.0f;
  static float arrTimer = 0.0f;
  static bool leftKeyPressed = false;
  static bool rightKeyPressed = false;

  bool moveLeft = false, moveRight = false;

  auto gamepad = FindGamepad();
  bool leftDown = IsKeyDown(KEY_LEFT);
  bool rightDown = IsKeyDown(KEY_RIGHT);

  if (gamepad != -1) {
    leftDown =
        leftDown || IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
    rightDown = rightDown ||
                IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
  }

  if (leftDown && !rightDown) {
    if (!leftKeyPressed) {
      leftKeyPressed = true;
      dasTimer = dasDelay;
      moveLeft = true;
    } else if (dasTimer <= 0) {
      if (arrTimer <= 0) {
        moveLeft = true;
        arrTimer = arrDelay;
      } else {
        arrTimer -= GetFrameTime();
      }
    } else {
      dasTimer -= GetFrameTime();
    }
  } else {
    leftKeyPressed = false;
  }

  if (rightDown && !leftDown) {
    if (!rightKeyPressed) {
      rightKeyPressed = true;
      dasTimer = dasDelay;
      moveRight = true;
    } else if (dasTimer <= 0) {
      if (arrTimer <= 0) {
        arrTimer = arrDelay;
        moveRight = true;
      } else {
        arrTimer -= GetFrameTime();
      }
    } else {
      dasTimer -= GetFrameTime();
    }
  } else {
    rightKeyPressed = false;
  }
  if (!IsKeyDown(KEY_LEFT) && !IsKeyDown(KEY_RIGHT)) {
    arrTimer = 0;
  }
  return HorizontalInput(moveLeft, moveRight);
}

Cell &Board::operator[](int x, int y) noexcept {
  if (rows.size() > y) {
    auto &row = rows[y];
    if (row.size() > x) {
      return row[x];
    }
  }
  // we are always bounds checked before calling this function.
  std::unreachable();
}

ShapeIndices Game::getIndices(std::unique_ptr<Tetromino> &tetromino) const {
  ShapeIndices indices;
  auto pattern = shapePatterns.at(tetromino->shape);
  for (const auto &idx : pattern) {
    const auto rotated = idx.rotated(tetromino->orientation);
    indices.push_back(tetromino->position + rotated);
  }
  return indices;
}

void Tetromino::spinRight() {
  auto ori = int(orientation);
  auto max_oris = 0;
  switch (shape) {
  case Shape::L:
    max_oris = 4;
    break;
  case Shape::J:
    max_oris = 4;
    break;
  case Shape::Z:
    max_oris = 2;
    break;
  case Shape::S:
    max_oris = 2;
    break;
  case Shape::I:
    max_oris = 2;
    break;
  case Shape::T:
    max_oris = 4;
    break;
  case Shape::O:
    max_oris = 1;
    break;
  }
  orientation = Orientation((ori + 1) % max_oris);
}

void Tetromino::spinLeft() {
  auto ori = int(orientation);
  auto max_oris = 0;
  switch (shape) {
  case Shape::L:
    max_oris = 4;
    break;
  case Shape::J:
    max_oris = 4;
    break;
  case Shape::Z:
    max_oris = 2;
    break;
  case Shape::S:
    max_oris = 2;
    break;
  case Shape::I:
    max_oris = 2;
    break;
  case Shape::T:
    max_oris = 4;
    break;
  case Shape::O:
    max_oris = 1;
    break;
  }
  orientation = Orientation((ori - 1 + max_oris) % max_oris);
}

void Tetromino::saveState() {
  prev_orientation = orientation;
  prev_position = position;
}
void Game::adjustScoreAndLevel(size_t linesCleared) {
  auto score_level = this->level + 1;

  if (linesCleared == 1) {
    score += 40 * score_level;
  } else if (linesCleared == 2) {
    score += 100 * score_level;
  } else if (linesCleared == 3) {
    score += 300 * score_level;
  } else if (linesCleared == 4) {
    score += 1200 * score_level;
  }

  totalLinesCleared += linesCleared;
  linesClearedThisLevel += linesCleared;

  auto levelAdvance = level == startLevel ? score_level * 10 : 10;

  if (linesClearedThisLevel >= levelAdvance) {
    level++;
    paletteIdx = (paletteIdx + 1) % (palette.size() - 1);

    printf("\033[1;32madvanced level: to %ld\033[0m\n", level);
    if (this->level < gravityLevels.size()) {
      gravity = gravityLevels[level];
    }
    linesClearedThisLevel = 0;
  }
}
size_t Game::clearLines(std::vector<size_t> &linesToClear) {
  if (linesToClear.empty()) {
    return 0;
  }

  for (auto idx : linesToClear) {
    animation_queue.push_back(std::make_unique<CellDissolveAnimation>(std::vector<size_t>(linesToClear)));
    animation_queue.push_back(std::make_unique<LineRemoveAnimation>(std::vector<size_t>(linesToClear)));
  }

  return linesToClear.size();
}
void BoardCell::draw(rayui::LayoutState &state) {
  if (!cell.empty) {
    auto color = game.palette[game.paletteIdx][cell.color];
    auto destRect = Rectangle{state.position.x, state.position.y,
                              state.size.width, state.size.height};
    DrawTexturePro(game.blockTexture, game.blockTxSourceRect, destRect, {0, 0},
                   0, color);
  }
};
void NumberText::draw(LayoutState &state) {
  DrawText(std::to_string(*number).c_str(), state.position.x, state.position.y,
           state.size.height, color);
}
void Game::drawGame() {

  if (!animation_queue.empty()) {
    animation_queue.front()->invoke();
    animation_queue.pop_front();
  }

  const auto screenWidth = (float)GetScreenWidth();
  const auto screenHeight = (float)GetScreenHeight();
  const auto unit = std::min(screenWidth / 26, screenHeight / 20);
  const auto uiWidth = unit * 26;
  const auto uiHeight = unit * 20;
  const auto posX = (screenWidth - uiWidth) / 2;
  const auto posY = (screenHeight - uiHeight) / 2;
  LayoutState state({posX, posY}, {uiWidth, uiHeight});
  gameGrid.draw(state);
}
std::shared_ptr<rayui::Grid> Game::createBoardGrid() {
  auto grid = std::make_shared<Grid>();
  grid->style.background = BLACK;
  grid->subdivisions = {10, 20};
  int y = 0;
  for (auto &row : board.rows) {
    int x = 0;
    for (auto &cell : row) {
      grid->emplace_element<BoardCell>(Position{x, y}, *this, cell);
      x++;
    }
    y++;
  }
  return grid;
}
void PieceViewer::draw(rayui::LayoutState &state) {
  DrawRectangle(state.position.x, state.position.y, state.size.width,
                state.size.height, style.background);

  auto blockSize = std::min(state.size.height / 2, state.size.width / 4);
  auto nextBlockAreaCenterX =
      state.position.x + state.size.height / 2 - blockSize;
  auto nextBlockAreaCenterY =
      state.position.y + state.size.width / 2 - blockSize;
  if (game.nextShape != Shape::I && game.nextShape != Shape::O) {
    nextBlockAreaCenterX += blockSize / 2;
  }
  if (game.nextShape == Shape::I) {
    nextBlockAreaCenterY += blockSize / 2;
  }

  for (const auto &block : game.shapePatterns.at(game.nextShape)) {
    auto color = game.palette[game.paletteIdx][game.nextColor];
    auto destX = nextBlockAreaCenterX + block.x * blockSize;
    auto destY = nextBlockAreaCenterY + block.y * blockSize;
    auto destRect = Rectangle{(float)destX, (float)destY, (float)blockSize,
                              (float)blockSize};
    DrawTexturePro(game.blockTexture, game.blockTxSourceRect, destRect, {0, 0},
                   0, color);
  }
};
Vec2 Vec2::operator+(const Vec2 &other) const {
  return {this->x + other.x, this->y + other.y};
}
Vec2 Vec2::rotated(Orientation orientation) const {
  switch (orientation) {
  case Orientation::Up:
    return *this;
  case Orientation::Left:
    return {-this->y, this->x};
  case Orientation::Down:
    return {-this->x, -this->y};
  case Orientation::Right:
    return {this->y, -this->x};
  }
}
std::string ScoreFile::getScoreFilePath() {
  std::string path;
#ifdef _WIN32
  char buffer[MAX_PATH];
  if (GetEnvironmentVariable("APPDATA", buffer, MAX_PATH)) {
    path = std::string(buffer) + "\\boom_tetris\\score";
  }
#else
  const char *home = getenv("HOME");
  if (home != nullptr) {
    path = std::string(home) + "/.config/boom_tetris/score";
  }
#endif
  createDirectoryAndFile(path);
  return path;
}
void ScoreFile::createDirectoryAndFile(const std::string &path) {
  try {
    std::filesystem::path dirPath = std::filesystem::path(path).parent_path();
    if (!std::filesystem::exists(dirPath)) {
      bool created = std::filesystem::create_directories(dirPath);
      if (created) {
        std::cout << "Directory created successfully: " << dirPath << std::endl;
      } else {
        std::cerr << "Failed to create directory: " << dirPath << std::endl;
        return;
      }
    }

    std::filesystem::path filePath = std::filesystem::path(path);
    if (!std::filesystem::exists(filePath)) {
      std::ofstream file(path);
      if (file) {
        std::cout << "File created successfully: " << filePath << std::endl;
      } else {
        std::cerr << "Failed to create file: " << filePath << std::endl;
      }
    }
  } catch (const std::filesystem::filesystem_error &e) {
    std::cerr << "Filesystem error: " << e.what() << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "General error: " << e.what() << std::endl;
  }
}
void ScoreFile::read() {
  std::string filename = getScoreFilePath();
  if (filename.empty()) {
    return;
  }
  std::ifstream file(filename);
  if (file.is_open()) {
    std::string s;
    file >> s;
    high_score = std::atoi(s.c_str());
    file.close();
  }
}
void ScoreFile::write() {
  std::string filename = getScoreFilePath();
  if (filename.empty()) {
    return;
  }
  std::ofstream file(filename);
  if (file.is_open()) {
    file << std::to_string(high_score);
    file.close();
  }
}
int Game::FindGamepad() const {
  for (int i = 0; i < 5; ++i) {
    if (IsGamepadAvailable(i))
      return i;
  }
  return -1;
}
void Game::generateGravityLevels(int totalLevels) {
  float divisor = 48.0;
  gravityLevels.push_back(1.0 / divisor);
  for (int level = 1; level < totalLevels; ++level) {
    if (level < 9) {
      divisor -= 5.0;
    } else if (level < 10) {
      divisor = 6;
    } else if (level < 13) {
      divisor = 5;
    } else if (level < 16) {
      divisor = 4;
    } else if (level < 19) {
      divisor = 3;
    } else if (level < 29) {
      divisor = 2;
    } else {
      divisor = 1;
    }
    gravityLevels.push_back(1.0 / divisor);
  }
}
