#include "raylib.h"

#include <algorithm>

#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// void draw() const {
//   for (int x = 0; x < size.x; ++x) {
//     for (int y = 0; y < size.y; ++y) {
// 			// source position on texture to sample from.
//       Rectangle sourceRec = {0.0f, 0.0f, (float)gTexture.width,
//                              (float)gTexture.height};

// 			// global position
// 			Vector2 globalPos {position.x + (x * UNIT), position.y +
// (y * UNIT)};

// 			// Where on the screen to draw this
//       Rectangle destRec{globalPos.x, globalPos.y,
//                         UNIT, UNIT};

// 			DrawRectangle(globalPos.x, globalPos.y, size.x * UNIT,
// size.y * UNIT, gPalette[gCurrentPalette][color + 1]);

// 			// texture, source, dest, origin, rotation, tint
//       DrawTexturePro(gTexture, sourceRec, destRec, {0,0}, rotation,
//                      gPalette[gCurrentPalette][color]);
//     }
//   }
// }

#define UNIT 32
#define BG_COLOR GetColor(0x12121212)
// a basic color palette. only one palette item right now.
const std::vector<std::vector<Color>> gPalette = {
    {BLUE, LIME, YELLOW, ORANGE, RED}};

// an integer Vector2
struct Vec2 {
  int x, y;
};

// a way to key into the grid to update a tetromino.
using ShapeIndices = std::vector<Vec2>;

// the block texture, used and tinted for every block.
Texture2D gTexture;

// an index into the current pallette, based on level.
size_t gCurrentPaletteIdx = 0;

// the direction of user input.
enum struct Direction { None, Left, Right, Down };

// at which rate are we moving the tetromino down?
float gGravity = 0.25f;
float gPlayerGravity = 0.0f;

// a grid cell.
struct Cell {
  size_t color = 0;
  bool empty = true;
};

struct Board {
  std::vector<std::vector<Cell>> rows;
  Cell &operator[](int x, int y) {
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

  auto begin() { return rows.begin(); }
  auto end() { return rows.end(); }

  template <typename... Args> auto &emplace_back(Args &&...args) {
    return rows.emplace_back(args...);
  }

  // We need more information that just whether it collided or not: we need to
  // know what side we hit so we can depenetrate in the opposite direction.
  bool collides(Vec2 pos) {
    int x = pos.x;
    int y = pos.y;
    if (y < 0 || y >= 20 || x < 0 || x >= 10) {
      // out of bounds. this should never happen
      // throw?
      printf("\e[31mout of bounds access into rows & columns in 'collides', x: "
             "%d, y: %d\e[0m\n",
             x, y);
      return false;
    }
    if (!rows[y][x].empty) {
      return true;
    }
    return false;
  }

  void draw() {
    static auto halfScreen = GetScreenWidth() / 2;
    static auto boardStart = halfScreen - (UNIT * 10 / 2);
    size_t x = 0, y = 0;
    for (const auto &row : rows) {
      for (const auto &cell : row) {
        auto color = BLACK;
        if (!cell.empty) {
          color = gPalette[gCurrentPaletteIdx][cell.color];
        }
        DrawRectangle(x + boardStart, y, UNIT, UNIT, color);
        x += UNIT;
      }
      y += UNIT;
      x = 0;
    }
  }

  void checkLines() {
    std::vector<size_t> linesToBurn = {};
    size_t i = 0;
    for (const auto &row : rows) {
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
        rows[j] = rows[j - 1];
      }
      for (auto &cell : rows[0]) {
        cell.empty = true;
      }
    }
  }
};

Board gBoard = {};

// the shape of a tetromino, a group of cells.
enum struct Shape { L, J, Z, S, I, T } shape;

// a static map of the coordinates of each shape in local space.
const std::unordered_map<Shape, std::vector<Vec2>> gShapePatterns = {
    {Shape::L, {{0, 0}, {0, 1}, {0, 2}, {1, 2}}},
    {Shape::J, {{1, 0}, {1, 1}, {1, 2}, {0, 2}}},
    {Shape::Z, {{0, 0}, {0, 1}, {1, 1}, {1, 2}}},
    {Shape::S, {{1, 0}, {1, 1}, {0, 1}, {0, 2}}},
    {Shape::I, {{0, 0}, {0, 1}, {0, 2}, {0, 3}}},
    {Shape::T, {{0, 1}, {1, 0}, {1, 1}, {1, 2}}}};

// a group of cells the user is currently in control of.
struct Tetromino {
  size_t color = 0;
  Vec2 last_pos;
  Vec2 pos;
  Shape shape;

  // discard old data. more specifically, write empty to all the cells that we
  // currently exist in, so that we can safely move into new cells.
  void clean() {
    auto indices = getIndices();
    for (const auto &idx : indices) {
      if (idx.y < 0 || idx.y >= 20 || idx.x < 0 || idx.x >= 10) {
        continue;
      }
      auto &cell = gBoard[idx.x, idx.y];
      cell.empty = true;
      cell.color = 0;
    }
  }

  // returns true if there was a collision, false otherwise
  bool resolveCollision() {
    for (const auto idx : getIndices()) {
      if (idx.y >= 20 || idx.x < 0 || idx.x >= 10 || gBoard.collides(idx)) {
        pos = last_pos;
        return true;
      }
    }
    return false;
  }

  ShapeIndices getIndices() const {
    ShapeIndices indices;
    auto pattern = gShapePatterns.at(shape);
    for (const auto &[dx, dy] : pattern) {
      indices.push_back({pos.x + dx, pos.y + dy});
    }
    return indices;
  }

  Tetromino() {
    int num_shapes = (int)Shape::T + 1;
    shape = Shape(rand() % num_shapes);
    color = (size_t)std::min((int)shape, 4);
    pos = {5, 0};
  }
};

Tetromino *gTetromino = nullptr;

void processGameLogic() {

  if (gTetromino == nullptr) {
    gTetromino = new Tetromino();
  }

  // DEBUG BUTTON: restart current piece.
  if (IsKeyPressed(KEY_R)) {
    if (gTetromino) {
      gTetromino->clean();
      delete gTetromino;
    }
    gTetromino = new Tetromino();
  }

  static float budge = 0.0;
  gTetromino->clean();

  if (IsKeyDown(KEY_LEFT)) {
    gTetromino->last_pos = gTetromino->pos;
    gTetromino->pos.x--;
    gTetromino->resolveCollision();
  }
  if (IsKeyDown(KEY_RIGHT)) {
    gTetromino->last_pos = gTetromino->pos;
    gTetromino->pos.x++;
    gTetromino->resolveCollision();
  }
  if (IsKeyDown(KEY_DOWN)) {
    gPlayerGravity = 0.25f;
  }

  gTetromino->last_pos = gTetromino->pos;
  budge += gGravity + gPlayerGravity;
  gPlayerGravity = 0.0f;
  auto floored = std::floor(budge);
  if (floored > 0) {
    gTetromino->pos.y += 1;
    budge = 0;
  }

  auto hit_bottom = gTetromino->resolveCollision();
  for (const auto &idx : gTetromino->getIndices()) {
    auto &cell = gBoard[idx.x, idx.y];
    cell.empty = false;
    cell.color = gTetromino->color;
  }

  if (hit_bottom) {
    delete gTetromino;
    gTetromino = nullptr;

    gBoard.checkLines();
  }
}
int main(int argc, char *argv[]) {

  InitWindow(800, 650, "boom taetris");
  gTexture = LoadTexture("res/block.png");
  SetTargetFPS(30);

  // initialze board grid.
  for (int y = 0; y < 20; ++y) {
    auto &n = gBoard.emplace_back();
    for (int x = 0; x < 10; ++x) {
      n.emplace_back();
    }
  }

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BG_COLOR);
    processGameLogic();
    gBoard.draw();
    EndDrawing();
  }

  return 0;
}