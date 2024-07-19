#include "tetris.hpp"
#include <cmath>
#include <stdexcept>
#include <string>

void Game::saveTetromino() { gTetromino->saveState(); }

Game::Game() {
  board = new Board();
  board->reset();
}
void Game::processGameLogic() {
  if (gTetromino == nullptr) {
    gTetromino = new Tetromino(*this);
    gTetromino->saveState();
    if (resolveCollision(*gTetromino)) {
      printf("game done\n");
      inMenu = true;
      return;
    }
  }
  
  static float budge = 0.0;
  cleanTetromino(*gTetromino);
  
  auto horizontal = delayedAutoShift();
  
  if (IsKeyPressed(KEY_Z)) {
    gTetromino->saveState();
    gTetromino->orientation = gTetromino->getPreviousOrientation();
    resolveCollision(*gTetromino);
  }
  if (IsKeyPressed(KEY_X) || IsKeyPressed(KEY_UP)) {
    gTetromino->saveState();
    gTetromino->orientation = gTetromino->getNextOrientation();
    resolveCollision(*gTetromino);
  }
  if (horizontal.left) {
    gTetromino->saveState();
    gTetromino->pos.x--;
    resolveCollision(*gTetromino);
  }
  if (horizontal.right) {
    gTetromino->saveState();
    gTetromino->pos.x++;
    resolveCollision(*gTetromino);
  }
  if (IsKeyDown(KEY_DOWN)) {
    playerGravity = 0.25f;
  }

  gTetromino->saveState();
  budge += gravity + playerGravity;
  playerGravity = 0.0f;
  auto floored = std::floor(budge);
  if (floored > 0) {
    gTetromino->pos.y += 1;
    budge = 0;
  }
  
  auto hit_bottom = resolveCollision(*gTetromino);
  for (const auto &idx : getIndices(*gTetromino)) {
    if (idx.x < 0 || idx.x >= 10 || idx.y < 0 || idx.y >= 20) {
      continue;
    }
    auto &cell = (*board)[idx.x, idx.y];
    cell.empty = false;
    cell.color = gTetromino->color;
  }
  
  if (hit_bottom) {
    delete gTetromino;
    gTetromino = nullptr;
    checkLines();
  }
}
void Game::setNextShapeAndColor() {
  static int num_shapes = (int)Shape::Square + 1;
  auto shape = Shape(rand() % num_shapes);
  *gNextShape = shape;
  *gNextColor = (size_t)std::min((int)shape, 4);
}
void Game::cleanTetromino(Tetromino &tetromino) {
    auto indices = getIndices(tetromino);
    for (const auto &idx : indices) {
      if (idx.y < 0 || idx.y >= 20 || idx.x < 0 || idx.x >= 10) {
        continue;
      }
      auto &cell = (*board)[idx.x, idx.y];
      cell.empty = true;
      cell.color = 0;
    }
}
void Game::checkLines() {
  std::vector<size_t> linesToBurn = {};
  size_t i = 0;
  for (const auto &row : board->rows) {
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

  for (auto idx : linesToBurn) {
    for (size_t j = idx; j > 0; --j) {
      board->rows[j] = board->rows[j - 1];
    }
    for (auto &cell : board->rows[0]) {
      cell.empty = true;
    }
  }
  
  auto level = this->level + 1;
  if (linesToBurn.size() == 1) {
    score += 40 * level;
  } else if (linesToBurn.size() == 2) {
    score += 100 * level;
  } else if (linesToBurn.size() == 3) {
    score += 300 * level;
  } else if (linesToBurn.size() == 4) {
    score += 1200 * level;
  }

  linesClearedThisLevel += linesToBurn.size();
  if (linesClearedThisLevel >= 10) {
    level++;
    if (level < gGravityLevels.size()) {
      gravity += gGravityLevels[level];
    }
    linesClearedThisLevel = 0;
  }
}
void Game::draw() {
  auto screen_width = GetScreenWidth();
  auto halfScreen = screen_width / 2;
  auto boardStart = halfScreen - (gBlockSize * 10 / 2);
  auto blockTxSourceRect =
      Rectangle{0, 0, (float)blockTexture.width, (float)blockTexture.height};
  size_t x = 0, y = 0;
  for (const auto &row : board->rows) {
    for (const auto &cell : row) {
      auto color = BLACK;
      if (!cell.empty) {
        color = palette[paletteIdx][cell.color];
        auto dx = x + boardStart;
        auto destRect = Rectangle{(float)dx, (float)y, (float)gBlockSize,
                                  (float)gBlockSize};
        DrawTexturePro(blockTexture, blockTxSourceRect, destRect, {0, 0},
                       0, color);
      } else {
        DrawRectangle(x + boardStart, y, gBlockSize, gBlockSize,
                      color);
      }
      
      x += gBlockSize;
    }
    y += gBlockSize;
    x = 0;
  }
}
void Game::drawUi() {
  auto blockTxSourceRect =
      Rectangle{0, 0, (float)blockTexture.width, (float)blockTexture.height};
  auto screen_width = GetScreenWidth();
  auto screen_height = GetScreenHeight();
  gBlockSize = std::min(screen_height / 20, screen_width / 26);
  auto halfScreen = screen_width / 2;
  auto leftStart = halfScreen - gBlockSize * 13;
  auto rightStart = halfScreen + gBlockSize * 5;
  DrawRectangle(leftStart, 0, gBlockSize * 26, gBlockSize * 20, DARKGRAY);
  // draw left side
  DrawText("Score:", leftStart + gBlockSize, 0, gBlockSize, WHITE);
  DrawText(std::to_string(score).c_str(), leftStart + gBlockSize, gBlockSize,
           gBlockSize, WHITE);
  DrawText("Level:", leftStart + gBlockSize, gBlockSize * 2, gBlockSize, WHITE);
  DrawText(std::to_string(level).c_str(), leftStart + gBlockSize,
           gBlockSize * 3, gBlockSize, WHITE);
  // draw right side
  DrawText("Next:", rightStart + gBlockSize, 0, gBlockSize, WHITE);
  DrawRectangle(rightStart + gBlockSize, gBlockSize, gBlockSize * 4,
                gBlockSize * 4, BLACK);
  auto nextBlockAreaCenterX = rightStart + gBlockSize * 2;
  auto nextBlockAreaCenterY = gBlockSize * 2;
  if (*gNextShape != Shape::I && *gNextShape != Shape::Square) {
    nextBlockAreaCenterX += gBlockSize / 2;
  }
  if (*gNextShape == Shape::I) {
    nextBlockAreaCenterY += gBlockSize / 2;
  }
  for (const auto &block : gShapePatterns.at(*gNextShape)) {
    auto color = palette[paletteIdx][*gNextColor];
    auto destX = nextBlockAreaCenterX + block.x * gBlockSize;
    auto destY = nextBlockAreaCenterY + block.y * gBlockSize;
    auto destRect = Rectangle{(float)destX, (float)destY, (float)gBlockSize,
                              (float)gBlockSize};
    DrawTexturePro(blockTexture, blockTxSourceRect, destRect, {0, 0}, 0, color);
  }
}
bool Game::resolveCollision(Tetromino &tetromino) {
  for (const auto idx : getIndices(tetromino)) {
    if (idx.y >= 20 || idx.x < 0 || idx.x >= 10 || board->collides(idx)) {
      tetromino.pos = tetromino.last_pos;
      tetromino.orientation = tetromino.last_ori;
      return true;
    }
  }
  return false;
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
bool Board::collides(Vec2 pos) {
  int x = pos.x;
  int y = pos.y;
  if (y < 0 || y >= 20 || x < 0 || x >= 10) {
    return false;
  }
  if (!rows[y][x].empty) {
    return true;
  }
  return false;
}
void Board::reset() {
  rows.clear();
  // initialze board grid.
  for (int y = 0; y < 20; ++y) {
    auto &n = emplace_back();
    for (int x = 0; x < 10; ++x) {
      n.emplace_back();
    }
  }
}
ShapeIndices Game::getIndices(Tetromino &tetromino) const {
  ShapeIndices indices;
  auto pattern = gShapePatterns.at(tetromino.shape);
  for (const auto &idx : pattern) {
    const auto rotated = tetromino.rotate(idx);
    indices.push_back(
        {tetromino.pos.x + rotated.x, tetromino.pos.y + rotated.y});
  }
  return indices;
}
