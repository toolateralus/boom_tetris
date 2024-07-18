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
enum struct Shape { L, J, Z, S, I, T } shape;

// a static map of the coordinates of each shape in local space.
const std::unordered_map<Shape, std::vector<std::pair<int, int>>>
    gShapePatterns = {{Shape::L, {{0, 0}, {0, 1}, {0, 2}, {1, 2}}},
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

  // returns false if the piece hasn't hit the bottom of the board,
  // true if it has.

  struct CollisionInfo {
    bool left = false,
		 		 right = false,
				 bottom = false,
				 block = false;
  };

  CollisionInfo resolveCollision() {

    CollisionInfo info = {false, false, false, false};

    for (const auto idx : getIndices()) {
      if (idx.y >= 20) {
        info.bottom = true;
      }
      if (idx.x < 0) {
        info.left = true;
      }
      if (idx.x >= 10) {
        info.right = true;
      }

      // this is not sophisticated enough.
      // one problem is: this throws errors when out of bounds.
      // the second issue, is that we can depenetrate upwards on a side
      // collision.
      if (gBoard.collides(idx)) {
				info.bottom = true;
      }
    }

    // if we hit the bottom, we've not resolved the Y collision
    bool resolved_y = !info.bottom;

    // resolve collisions with the floor
    while (!resolved_y) {
      pos.y -= 1;

      resolved_y = true;
      for (const auto idx : getIndices()) {
        if (idx.y >= 20) {
          resolved_y = false;
          break;
        }
      }
    }

    // if we've hit either of the walls, we need to do a horizontal collision
    // resolution.
    bool resolved_x = !(info.left || info.right);
    
    while (!resolved_x) {
      if (info.left) {
        pos.x += 1;
      } else {
        pos.x -= 1;
      }

      resolved_x = true;
      for (const auto idx : getIndices()) {
        if (idx.x < 0 || idx.x >= 10) {
          resolved_x = false;
          break;
        }
      }
    }
    return info;
  }

  bool move(Direction dir) {
    // this float increments each time gravity is applied
    // and gets set back to zero when it exceeds one.
    // this is to get sub-frame velocity in a grid
    static float budge = 0.0;

    last_pos = pos;

    clean();

    if (dir == Direction::Left) {
      pos.x--;
    } else if (dir == Direction::Right) {
      pos.x++;
    }

    if (dir == Direction::Down) {
      gPlayerGravity = 0.25f;
    }

    // apply downward force.
    budge += gGravity + gPlayerGravity;
    gPlayerGravity = 0.0f;
    auto floored = std::floor(budge);
    if (floored > 0) {
      pos.y += 1;
      budge = 0;
    }

    auto collision = resolveCollision();
    
    // now we are completely in bounds, we draw our cells.
    for (const auto &idx : getIndices()) {
      auto &cell = gBoard[idx.x, idx.y];
      cell.empty = false;
      cell.color = color;
    }
    
    return collision.bottom;
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

  auto hit_bottom = false;

  bool moved = false;
  if (IsKeyDown(KEY_LEFT) && !hit_bottom) {
    hit_bottom = gTetromino->move(Direction::Left);
    moved = true;
  }
  if (IsKeyDown(KEY_RIGHT) && !hit_bottom) {
    hit_bottom = gTetromino->move(Direction::Right);
    moved = true;
  }
  if (IsKeyDown(KEY_DOWN) && !hit_bottom) {
    hit_bottom = gTetromino->move(Direction::Down);
    moved = true;
  }

  if (!moved) {
    hit_bottom = gTetromino->move(Direction::None);
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