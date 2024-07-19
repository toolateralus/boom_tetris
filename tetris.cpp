#include "tetris.hpp"
#include <cmath>
#include <functional>
#include <raylib.h>
#include <stdexcept>
#include <string>

void Game::saveTetromino() { tetromino->saveState(); }

std::vector<float> Game::gravityLevels = {
    1.0 / 48, 1.0 / 43, 1.0 / 38, 1.0 / 33, 1.0 / 28,
    1.0 / 23, 1.0 / 18, 1.0 / 13, 1.0 / 8,  1.0 / 6,
    1.0 / 6,  1.0 / 6,  1.0 / 6,  1.0 / 5,  1.0 / 5,
};

std::vector<std::vector<Color>> Game::palette = {
    {BLUE, LIME, YELLOW, ORANGE, RED}};

std::unordered_map<Shape, std::vector<Vec2>> Game::shapePatterns = {
    {Shape::L, {{-1, 1}, {-1, 0}, {0, 0}, {1, 0}}},
    {Shape::J, {{-1, 0}, {0, 0}, {1, 0}, {1, 1}}},
    {Shape::Z, {{-1, 0}, {0, 0}, {0, 1}, {1, 1}}},
    {Shape::S, {{-1, 1}, {0, 1}, {0, 0}, {1, 0}}},
    {Shape::I, {{-1, 0}, {0, 0}, {1, 0}, {2, 0}}},
    {Shape::T, {{-1, 0}, {0, 0}, {1, 0}, {0, 1}}},
    {Shape::Square, {{0, 0}, {0, 1}, {1, 0}, {1, 1}}},
};

Game::Game() {
  board = Board();
  setNextShapeAndColor();
  blockTexture = LoadTexture("res/block.png");
  inMenu = true; 
}

void Game::processGameLogic() {
  
  if (!tetromino) {
    // spawn a new tetromino, cause the last one landed.
    auto shape = nextShape;
    auto color = nextColor;
    tetromino = std::make_unique<Tetromino>(shape, color);
    setNextShapeAndColor();
    
    tetromino->saveState();
    if (resolveCollision(tetromino)) {
      inMenu = true;
      return;
    }
  }

  static float budge = 0.0;
  
  cleanTetromino(tetromino);

  auto horizontal = delayedAutoShift();
  
  auto executeMovement = [&](std::function<void()> fun) -> bool {
    tetromino->saveState();
    fun();
    return resolveCollision(tetromino);
  };

  if (IsKeyPressed(KEY_Z)) {
    executeMovement([&]() { tetromino->spinLeft(); });
  }
  if (IsKeyPressed(KEY_X) || IsKeyPressed(KEY_UP)) {
    executeMovement([&] { tetromino->spinRight(); });
  }
  if (horizontal.left) {
    executeMovement([&] { tetromino->position.x--; });
  }
  if (horizontal.right) {
    executeMovement([&] { tetromino->position.x++; });
  }
  if (IsKeyDown(KEY_DOWN)) {
    playerGravity = 0.25f;
  }
  
  // check if this piece hit the floor, or another piece.  
  auto landed = executeMovement([&] {
    budge += gravity + playerGravity;
    playerGravity = 0.0f;
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
}

void Game::setNextShapeAndColor() {
  static int num_shapes = (int)Shape::Square + 1;
  auto shape = Shape(rand() % num_shapes);
  nextShape = shape;
  nextColor = (size_t)std::min((int)shape, 4);
  
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

void Game::draw() {
  auto screen_width = GetScreenWidth();
  auto halfScreen = screen_width / 2;
  auto boardStart = halfScreen - (blockSize * boardWidth / 2);
  auto blockTxSourceRect =
      Rectangle{0, 0, (float)blockTexture.width, (float)blockTexture.height};
  size_t x = 0, y = 0;

  for (const auto &row : board.rows) {
    for (const auto &cell : row) {
      auto color = BLACK;
      if (!cell.empty) {
        color = palette[paletteIdx][cell.color];
        auto dx = x + boardStart;
        auto destRect = Rectangle{(float)dx, (float)y, (float)blockSize,
                                  (float)blockSize};
        DrawTexturePro(blockTexture, blockTxSourceRect, destRect, {0, 0}, 0,
                       color);
      } else {
        DrawRectangle(x + boardStart, y, blockSize, blockSize, color);
      }

      x += blockSize;
    }
    y += blockSize;
    x = 0;
  }
}

void Game::drawUi() {
  // constants
  constexpr auto uiWidth = 26;
  constexpr auto uiHeight = (int)boardHeight;
  constexpr auto halfUiWidth = uiWidth / 2;
  constexpr auto halfBoardWidth = boardWidth / 2;

  const auto blockTxSourceRect =
      Rectangle{0, 0, (float)blockTexture.width, (float)blockTexture.height};
  const auto screenWidth = GetScreenWidth();
  const auto screenHeight = GetScreenHeight();
  blockSize = std::min(screenHeight / uiHeight, screenWidth / uiWidth);
  const auto halfScreen = screenWidth / 2;
  const auto leftStart = halfScreen - blockSize * halfUiWidth;
  const auto rightStart = halfScreen + blockSize * halfBoardWidth;

  // draw left side
  DrawText("Score:", leftStart + blockSize, 0, blockSize, WHITE);
  DrawText(std::to_string(score).c_str(), leftStart + blockSize, blockSize,
           blockSize, WHITE);
           
  DrawText("Level:", leftStart + blockSize, blockSize * 2, blockSize, WHITE);
  DrawText(std::to_string(level).c_str(), leftStart + blockSize,
           blockSize * 3, blockSize, WHITE);
           
  // draw right side
  DrawText("Next:", rightStart + blockSize, 0, blockSize, WHITE);
  
  DrawRectangle(rightStart + blockSize, blockSize, blockSize * 4,
                blockSize * 4, BLACK);
                
  auto nextBlockAreaCenterX = rightStart + blockSize * 2;
  auto nextBlockAreaCenterY = blockSize * 2;
  if (nextShape != Shape::I && nextShape != Shape::Square) {
    nextBlockAreaCenterX += blockSize / 2;
  }
  if (nextShape == Shape::I) {
    nextBlockAreaCenterY += blockSize / 2;
  }
  
  for (const auto &block : shapePatterns.at(nextShape)) {
    auto color = palette[paletteIdx][nextColor];
    auto destX = nextBlockAreaCenterX + block.x * blockSize;
    auto destY = nextBlockAreaCenterY + block.y * blockSize;
    auto destRect = Rectangle{(float)destX, (float)destY, (float)blockSize,
                              (float)blockSize};
    DrawTexturePro(blockTexture, blockTxSourceRect, destRect, {0, 0}, 0, color);
  }
}

bool Game::resolveCollision(std::unique_ptr<Tetromino> &tetromino) {
  for (const auto idx : getIndices(tetromino)) {
    if (idx.y >= boardHeight || idx.x < 0 || idx.x >= boardWidth || board.collides(idx)) {
      tetromino->position = tetromino->prev_position;
      tetromino->orientation = tetromino->prev_orientation;
      return true;
    }
  }
  return false;
}

bool Board::collides(Vec2 pos) {
  int x = pos.x;
  int y = pos.y;
  if (y < 0 || y >= boardHeight || x < 0 || x >= boardWidth) {
    return false;
  }
  if (!rows[y][x].empty) {
    return true;
  }
  return false;
}

Game::~Game() { UnloadTexture(blockTexture); }

void Game::reset() {
  score = 0;
  gravity =  gravityLevels[level];
  linesClearedThisLevel = 0;
  board = {}; // reset the grid state.
  gravity = 0.1f;
  tetromino = nullptr;
  setNextShapeAndColor();
}

HorizontalInput Game::delayedAutoShift() {
  static float dasDelay = 0.2f;
  static float arrDelay = 0.05f;
  static float dasTimer = 0.0f;
  static float arrTimer = 0.0f;
  static bool leftKeyPressed = false;
  static bool rightKeyPressed = false;

  bool moveLeft = false, moveRight = false;

  if (IsKeyDown(KEY_LEFT)) {
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

  if (IsKeyDown(KEY_RIGHT)) {
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

Cell &Board::operator[](int x, int y) {
  if (rows.size() > y) {
    auto &row = rows[y];
    if (row.size() > x) {
      return row[x];
    }
  }
  throw std::runtime_error(
      "invalid indices into Board: (x=" + std::to_string(x) +
      ", y=" + std::to_string(y) + ")");
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
  case Shape::Square:
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
  case Shape::Square:
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
  
  linesClearedThisLevel += linesCleared;
  
  if (linesClearedThisLevel >= boardWidth) {
    level++;
    printf("\033[1;32madvanced level: to %ld\033[0m\n", level);
    if (score_level < gravityLevels.size()) {
      gravity = gravityLevels[level];
    }
    linesClearedThisLevel = 0;
  }
}
size_t Game::clearLines(std::vector<size_t> &linesToClear) {
  for (auto idx : linesToClear) {
    for (size_t j = idx; j > 0; --j) {
      board.rows[j] = board.rows[j - 1];
    }
    for (auto &cell : board.rows[0]) {
      cell.empty = true;
    }
  }
  return linesToClear.size();
}
