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
  std::vector<std::vector<Cell>> columns;
  Cell &operator[](int x, int y) {
    if (columns.size() > y) {
      auto &row = columns[y];
      if (row.size() > x) {
        return row[x];
      }
    }
    throw std::runtime_error(
        "invalid indices into Board: (x=" + std::to_string(x) +
        ", y=" + std::to_string(y) + ")");
  }

  auto begin() { return columns.begin(); }
  auto end() { return columns.end(); }

  template <typename... Args> auto &emplace_back(Args &&...args) {
    return columns.emplace_back(args...);
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
    if (!columns[y][x].empty) {
      return true;
    }
    return false;
  }

  void draw() {
    static auto halfScreen = GetScreenWidth() / 2;
    static auto boardStart = halfScreen - (UNIT * 10 / 2);
    size_t x = 0, y = 0;
    for (const auto &row : columns) {
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
};

Board gBoard = {};

// the shape of a tetromino, a group of cells.
enum struct Shape { L, J, Z, S, I, T };
enum struct Orientation { Up, Right, Down, Left };


// a static map of the coordinates of each shape in local space.
const std::unordered_map<Shape, std::vector<Vec2>>
    gShapePatterns = {{Shape::L, {{ 0,-1}, { 0, 0}, { 0, 1}, { 1, 1}}},
                      {Shape::J, {{ 0,-1}, { 0, 0}, { 0, 1}, {-1, 1}}},
                      {Shape::Z, {{ 0,-1}, { 0, 0}, { 1, 0}, { 1, 1}}},
                      {Shape::S, {{ 0,-1}, { 0, 0}, {-1, 0}, {-1, 1}}},
                      {Shape::I, {{ 0,-1}, { 0, 0}, { 0, 1}, { 0, 2}}},
                      {Shape::T, {{-1, 1}, { 0, 0}, { 0, 1}, { 0, 2}}}};

// a group of cells the user is currently in control of.
struct Tetromino {
  size_t color = 0;
  Vec2 last_pos;
  Orientation last_ori;
  Vec2 pos;
  Shape shape;
  Orientation orientation = Orientation::Up;

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
        orientation = last_ori;
        return true;
      }
    }
    return false;
  }

  Orientation getNextOrientation() const {
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
    // TODO: Add squares!!
    }
    return Orientation((ori + 1) % max_oris);
  }

  Orientation getPreviousOrientation() const {
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
    // TODO: Add squares!!
    }
    return Orientation((ori - 1 + max_oris) % max_oris);
  }

  Vec2 rotate(Vec2 input) const {
    switch (orientation) {
    case Orientation::Up:
      return input;
    case Orientation::Left:
      return {-input.y, input.x};
    case Orientation::Down:
      return {-input.x, -input.y};
    case Orientation::Right:
      return {input.y, -input.x};
    }
  }

  ShapeIndices getIndices() const {
    ShapeIndices indices;
    auto pattern = gShapePatterns.at(shape);
    for (const auto& idx : pattern) {
      const auto rotated = rotate(idx);
      indices.push_back({pos.x + rotated.x, pos.y + rotated.y});
    }
    return indices;
  }
  void saveState() {
    last_ori = orientation;
    last_pos = pos;
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

  if (IsKeyPressed(KEY_Z)) {
    gTetromino->saveState();
    gTetromino->orientation = gTetromino->getPreviousOrientation();
    gTetromino->resolveCollision();
  }
  if (IsKeyPressed(KEY_X)) {
    gTetromino->saveState();
    gTetromino->orientation = gTetromino->getNextOrientation();
    gTetromino->resolveCollision();
  }
  if (IsKeyDown(KEY_LEFT)) {
    gTetromino->saveState();
    gTetromino->pos.x--;
    gTetromino->resolveCollision();
  }
  if (IsKeyDown(KEY_RIGHT)) {
    gTetromino->saveState();
    gTetromino->pos.x++;
    gTetromino->resolveCollision();
  }
  if (IsKeyDown(KEY_DOWN)) {
    gPlayerGravity = 0.25f;
  }
  
  gTetromino->saveState();
  budge += gGravity + gPlayerGravity;
  gPlayerGravity = 0.0f;
  auto floored = std::floor(budge);
  if (floored > 0) {
    gTetromino->pos.y += 1;
    budge = 0;
  }

  auto hit_bottom = gTetromino->resolveCollision();
  for (const auto &idx : gTetromino->getIndices()) {
    if (idx.x < 0 || idx.x >= 10 || idx.y < 0 || idx.y >= 20) {
      continue;
    }
    auto &cell = gBoard[idx.x, idx.y];
    cell.empty = false;
    cell.color = gTetromino->color;
  }

  if (hit_bottom) {
    delete gTetromino;
    gTetromino = nullptr;
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