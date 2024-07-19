#include "raylib.h"

#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include <unordered_map>
#include <vector>
#pragma once

// an integer Vector2
#include <cstddef>
#include <vector>
struct Vec2 {
  int x, y;
};

// a way to key into the grid to update a tetromino.
using ShapeIndices = std::vector<Vec2>;
// the direction of user input.
enum struct Direction { None, Left, Right, Down };
// the shape of a tetromino, a group of cells.
enum struct Shape { L, J, Z, S, I, T, Square };
enum struct Orientation { Up, Right, Down, Left };

#define BG_COLOR GetColor(0x12121212)
// a basic color palette. only one palette item right now.
struct HorizontalInput {
  HorizontalInput(bool left, bool right) : left(left), right(right) {}
  bool left, right;
};

struct Tetromino;
struct Board;
struct Game {
  static std::vector<float> gGravityLevels;
  
  Board *board;
  Shape nextShape = (Shape)-1;
  int nextColor = -1;
  
  Tetromino *gTetromino = nullptr;

  void reset();

  Game();
  ~Game();

  static std::unordered_map<Shape, std::vector<Vec2>> gShapePatterns;

  int gBlockSize = 32;
   std::vector<std::vector<Color>> palette = {
      {BLUE, LIME, YELLOW, ORANGE, RED}};
  // the block texture, used and tinted for every block.
  Texture2D blockTexture;
  // an index into the current pallette, based on level.
  size_t paletteIdx = 0;
  // at which rate are we moving the tetromino down?
  float gravity = 0.15f;
  float playerGravity = 0.0f;
  size_t level = 0;
  size_t score = 0;
  size_t linesClearedThisLevel = 0;
  bool inMenu = true;
  
  void setNextShapeAndColor();
  void processGameLogic();
  void drawUi();
  void draw();
  void checkLines();
  void saveTetromino();
  void cleanTetromino(Tetromino *tetromino);
  bool resolveCollision(Tetromino *tetromino);
  HorizontalInput delayedAutoShift();
  ShapeIndices getIndices(Tetromino *tetromino) const;
};

// a grid cell.
struct Cell {
  size_t color = 0;
  bool empty = true;
};

struct Board {
  std::vector<std::vector<Cell>> rows;
  Cell &operator[](int x, int y);
  auto begin() { return rows.begin(); }
  auto end() { return rows.end(); }
  template <typename... Args> auto &emplace_back(Args &&...args) {
    return rows.emplace_back(args...);
  }
  // We need more information that just whether it collided or not: we need to
  // know what side we hit so we can depenetrate in the opposite direction.
  bool collides(Vec2 pos);
  void reset();
};

// a group of cells the user is currently in control of.
struct Tetromino {
  size_t color = 0;
  Vec2 last_pos;
  Orientation last_ori;
  Vec2 pos;
  Shape shape;
  Orientation orientation = Orientation::Up;
  // currently exist in, so that we can safely move into new cells.
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
    case Shape::Square:
      max_oris = 1;
      break;
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
    case Shape::Square:
      max_oris = 1;
      break;
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
  void saveState() {
    last_ori = orientation;
    last_pos = pos;
  }
  
  Tetromino() = delete;
  Tetromino(Shape &shape, size_t color) {
    this->shape = shape;
    this->color = color;
    pos = {5, 0};
  }
};

