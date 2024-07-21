#include "tetris.hpp"
#include "rayui.hpp"
#include <chrono>
#include <cmath>

#include <functional>
#include <iostream>
#include <memory>
#include <raylib.h>
#include <stdexcept>
#include <string>

using namespace boom_tetris;

void Game::saveTetromino() { tetromino->saveState(); }

std::unordered_map<Shape, std::vector<Block>> Game::shapePatterns = {
    {Shape::L, {{{-1, 1}, 1}, {{-1, 0}, 1}, {{0, 0}, 1}, {{1, 0}, 1}}},
    {Shape::J, {{{-1, 0}, 0}, {{0, 0}, 0}, {{1, 0}, 0}, {{1, 1}, 0}}},
    {Shape::Z, {{{-1, 0}, 1}, {{0, 0}, 1}, {{0, 1}, 1}, {{1, 1}, 1}}},
    {Shape::S, {{{-1, 1}, 0}, {{0, 1}, 0}, {{0, 0}, 0}, {{1, 0}, 0}}},
    {Shape::I, {{{-1, 0}, 3}, {{0, 0}, 3}, {{1, 0}, 3}, {{2, 0}, 3}}},
    {Shape::T, {{{-1, 0}, 3}, {{0, 0}, 3}, {{1, 0}, 3}, {{0, 1}, 3}}},
    {Shape::O, {{{0, 0}, 2}, {{0, 1}, 2}, {{1, 0}, 2}, {{1, 1}, 2}}},
};

Game::Game() {
  board = Board();
  setNextShape();
  blockTexture = LoadTexture("res/block2.png");
  shiftSound = LoadSound("res/shift.wav");
  rotateSound = LoadSound("res/rotate.wav");
  lockInSound = LoadSound("res/lock.wav");
  clearLineSound = LoadSound("res/clear.wav");
  gameGrid = createGrid();
  scene = Scene::MainMenu;
  scoreFile.read();
  generateGravityLevels(100);
}

void Game::processGameLogic() {
  elapsed += std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(1)) / 60;
  
  frameCount++;
  // if an animation is active, we pause the game.
  if (!animation_queue.empty()) {
    return;
  }

  if (!tetromino) {
    // spawn a new tetromino, cause the last one landed.
    auto shape = nextShape;
    tetromino = std::make_unique<Tetromino>(shape);
    setNextShape();

    tetromino->saveState();

    // game-over condition. currently, this is premature sometimes.
    if (resolveCollision(tetromino)) {
      if (score > scoreFile.high_score) {
        scoreFile.high_score = score;
      }
      scene = Scene::GameOver;
      return;
    }
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

  if (auto gpad = findGamepad(); gpad != -1) {
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
    if (!executeMovement([&]() { tetromino->spinLeft(); })) {
      PlaySound(rotateSound);
    }
  }

  if (turnRight) {
    if (!executeMovement([&] { tetromino->spinRight(); })) {
      PlaySound(rotateSound);
    }
  }

  if (horizontal.left) {
    if (!executeMovement([&] { tetromino->position.x--; })) {
      PlaySound(shiftSound);
    }
  }
  if (horizontal.right) {
    if (!executeMovement([&] { tetromino->position.x++; })) {
      PlaySound(shiftSound);
    }
  }

  auto oldGravity = gravity;
  if (moveDown) {
    if (gravity <= 0.5f) {
      gravity = 0.5;
    }
  } else {
    tetromino->softDropHeight = 0;
  }

  // check if this piece hit the floor, or another piece.
  auto landed = executeMovement([&] {
    budge += gravity;
    auto floored = std::floor(budge);
    if (floored > 0) {
      if (moveDown) {
        tetromino->softDropHeight++;
      }
      tetromino->position.y += 1;
      budge = 0;
    }
  });

  for (const auto &block : getTransformedBlocks(tetromino)) {
    auto pos = block.pos;
    if (pos.x < 0 || pos.x >= boardWidth || pos.y < 0 || pos.y >= boardHeight) {
      continue;
    }
    auto &cell = board.get_cell(pos.x, pos.y);
    cell.empty = false;
    cell.imageIdx = block.imageIdx;
  }

  // if we landed, we leave the cells where they are and spawn a new piece.
  // also check for line clears and tetrises.
  if (landed) {
    animation_queue.push_back(std::make_unique<LockInAnimation>(this, tetromino->position.y));
    auto linesToClear = checkLines();
    if (linesToClear.size() > 0) {
      ::PlaySound(clearLineSound);
      animation_queue.push_back(std::make_unique<CellDissolveAnimation>(this, linesToClear, tetromino->softDropHeight));
    } else {
      ::PlaySound(lockInSound);
      applySoftDropScore(tetromino->softDropHeight);
    }
    tetromino.reset(nullptr);
    if (mode == Mode::FortyLines && totalLinesCleared >= 40) {
      scene = Scene::GameOver;
    }
  }

  gravity = oldGravity;
}

void Game::setNextShape() {
  static int num_shapes = (int)Shape::O + 1;
  auto shape = Shape(rand() % num_shapes);
  nextShape = shape;
}

void Game::cleanTetromino(std::unique_ptr<Tetromino> &tetromino) {
  auto indices = getTransformedBlocks(tetromino);
  for (const auto &block : indices) {
    auto pos = block.pos;
    if (pos.y < 0 || pos.y >= boardHeight || pos.x < 0 || pos.x >= boardWidth) {
      continue;
    }
    auto &cell = board.get_cell(pos.x, pos.y);
    cell.empty = true;
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
      grid.emplace_element<Label>(Position{1, 1}, Size{7, 1});
  linesLabel->text = "Lines:";
  grid.emplace_element<NumberText>(Position{1, 2}, Size{7, 1},
                                   &totalLinesCleared, WHITE);
  
  if (mode == Mode::FortyLines) {
    auto timer_label = grid.emplace_element<Label>(Position{1, 4}, Size{5, 1});
    timer_label->text = "Timer";
    auto timer_text = grid.emplace_element<TimeText>(Position{1, 5}, Size{1, 1}, &elapsed, WHITE);
  }
  
  
  auto playfield = createBoardGrid();
  playfield->position = {8, 0};
  playfield->size = {10, 20};
  grid.elements.push_back(playfield);
  
  int yPos = 1, height = 1;
  
  
  
  auto topLabel =
      grid.emplace_element<Label>(Position{19, yPos}, Size{7, height});
  yPos += height;
  topLabel->text = "Top:";
  grid.emplace_element<NumberText>(Position{19, yPos}, Size{7, height},
                                   &scoreFile.high_score, WHITE);
  yPos += height;
  yPos += 1;

  auto scoreLabel =
      grid.emplace_element<Label>(Position{19, yPos}, Size{7, height});
  yPos += height;
  scoreLabel->text = "Score:";
  grid.emplace_element<NumberText>(Position{19, yPos}, Size{7, height}, &score,
                                   WHITE);
  yPos += height;
  yPos += 1;

  auto nextPieceLabel =
      grid.emplace_element<Label>(Position{19, yPos}, Size{7, height});
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
      grid.emplace_element<Label>(Position{19, yPos}, Size{7, height});
  yPos += height;
  levelLabel->text = "Level:";
  grid.emplace_element<NumberText>(Position{19, yPos}, Size{7, height}, &level,
                                   WHITE);
  yPos += height;
  yPos += 1;
  

  auto mainMenuButton = grid.emplace_element<Button>(
      Position{19, yPos}, Size{5, height}, "Main Menu",
      std::function<void()>([&]() { scene = Scene::MainMenu; }));
  mainMenuButton->fontSize = 24;
  yPos += height;
  
  // somehow this causes a crash (pressing the reset button, not making it) and windows is impossible to debug so I can't even breakpoint or find where it happens.
  // Somewhere in a windows api.
  // pressing main menu and re-entering is easy enough.
  #ifndef _WIN32
  auto resetButton =
      grid.emplace_element<Button>(Position{19, yPos}, Size{5, height}, "Reset",
                                   std::function<void()>([&]() { reset(); }));
  resetButton->fontSize = 24;
  yPos += height;
  #endif

  return grid;
}

bool Game::resolveCollision(std::unique_ptr<Tetromino> &tetromino) {
  for (const auto block : getTransformedBlocks(tetromino)) {
    auto pos = block.pos;
    if (
      pos.y >= boardHeight ||
      pos.x < 0 ||
      pos.x >= boardWidth ||
      board.collides(pos)
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
  animation_queue.clear();
  frameCount = 0;
  mode = Mode::Normal;
  level = startLevel;
  gravity = gravityLevels[level];
  linesClearedThisLevel = 0;
  totalLinesCleared = 0;
  board = {}; // reset the grid state.
  elapsed = {};
  tetromino = nullptr;

  setNextShape();
  gameGrid = createGrid();
}

HorizontalInput Game::delayedAutoShift() {
  static float dasDelay = 0.26667f;
  static float arrDelay = 0.070f;
  static float dasTimer = 0.0f;
  static float arrTimer = 0.0f;
  static bool leftKeyPressed = false;
  static bool rightKeyPressed = false;

  bool moveLeft = false, moveRight = false;

  auto gamepad = findGamepad();
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

Cell &Board::operator[](int x, int y)  {
  if (rows.size() > y) {
    auto &row = rows[y];
    if (row.size() > x) {
      return row[x];
    }
  }
  throw std::runtime_error("board subscript out of range");
}

ShapeIndices Game::getTransformedBlocks(std::unique_ptr<Tetromino> &tetromino) const {
  ShapeIndices indices;
  auto pattern = shapePatterns.at(tetromino->shape);
  for (const auto &block : pattern) {
    const auto rotated = block.pos.rotated(tetromino->orientation);
    indices.push_back({tetromino->position + rotated, block.imageIdx});
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
void Game::applyLineClearScoreAndLevel(size_t linesCleared) {
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
    if (this->level < gravityLevels.size()) {
      gravity = gravityLevels[level];
    }
    linesClearedThisLevel = 0;
  }
}
void BoardCell::draw(rayui::LayoutState &state) {
  if (!cell.empty) {
    auto destRect = Rectangle{state.position.x, state.position.y,
                              state.size.width, state.size.height};
    Rectangle srcRect = {(float)cell.imageIdx * 8, (float)(game.level % 10) * 8, 8, 8};
    DrawTexturePro(game.blockTexture, srcRect, destRect, {0, 0}, 0, WHITE);
  }
};

void Game::drawGame() {

  if (!animation_queue.empty()) {
    if (animation_queue.front()->invoke()) {
      animation_queue.pop_front();
    }
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
    auto destX = nextBlockAreaCenterX + block.pos.x * blockSize;
    auto destY = nextBlockAreaCenterY + block.pos.y * blockSize;
    auto destRect = Rectangle{(float)destX, (float)destY, (float)blockSize,
                              (float)blockSize};
    Rectangle srcRect = {(float)block.imageIdx * 8, (float)(game.level % 10) * 8, 8, 8};
    DrawTexturePro(game.blockTexture, srcRect, destRect, {0, 0}, 0, WHITE);
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
int Game::findGamepad() const {
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
bool CellDissolveAnimation::invoke() {
  if (cellIdx >= 5) {
    for (const auto line : lines) {
      for (int j = line; j >= 1; j--) {
        game->board.rows[j] = game->board.rows[j - 1];
      }
    }
    game->applyLineClearScoreAndLevel(lines.size());
    return true;
  }
  if (game->frameCount % 4 == 0) {
    for (const auto line : lines) {
      game->board[4 - cellIdx, line].empty = true;
      game->board[5 + cellIdx, line].empty = true;
    }
    cellIdx++;
  }
  return false;
}
bool LockInAnimation::invoke() {
  if (frameCount == 10 + ((20 - pieceHeight) / 4) * 2) {
    return true;
  }
  frameCount++;
  return false;
};
void Game::applySoftDropScore(size_t softDropHeight) {
  auto softDropScore = softDropHeight % 16;
  softDropScore += (softDropHeight / 16) % 16 * 10;
  score += softDropScore;
};