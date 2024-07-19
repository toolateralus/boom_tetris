#include "raylib.h"

#include <cstdlib>
#include <ctime>
#include <memory>
#include <stdio.h>
#include <unordered_map>
#include <vector>
#pragma once

// an integer Vector2
#include <cstddef>
#include <vector>


// the direction of user input.
enum struct Direction { None, Left, Right, Down };
// the shape of a tetromino, a group of cells.
enum struct Shape { L, J, Z, S, I, T, Square };
// the rotation of a tetromino
enum struct Orientation { Up, Right, Down, Left };

struct Vec2 {
  int x, y;
  
  Vec2 operator+(const Vec2 &other) const {
    return {this->x + other.x, this->y + other.y};
  }
  
  Vec2 rotated(Orientation orientation) const {
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
};

// a way to key into the grid to update a tetromino.
using ShapeIndices = std::vector<Vec2>;

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
  
  std::unique_ptr<Board> board;
  Shape nextShape = (Shape)-1;
  int nextColor = -1;
  
  std::unique_ptr<Tetromino> tetromino;
  
  void reset();

  Game();
  ~Game();

  static std::unordered_map<Shape, std::vector<Vec2>> gShapePatterns;

  int gBlockSize = 32;
  static std::vector<std::vector<Color>> palette;
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
  void cleanTetromino(std::unique_ptr<Tetromino> &tetromino);
  bool resolveCollision(std::unique_ptr<Tetromino> &tetromino);
  HorizontalInput delayedAutoShift();
  ShapeIndices getIndices(std::unique_ptr<Tetromino> &tetromino) const;
};

// a grid cell.
struct Cell {
  size_t color = 0;
  bool empty = true;
};

struct Board {
  std::vector<std::vector<Cell>> rows;
  Cell &operator[](int x, int y);
  
  Cell &get_cell(int x, int y) {
    return (*this)[x,y];
  }
  
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
  void spinRight();
  void spinLeft();
  void saveState();
  
  Tetromino() = delete;
  Tetromino(Shape &shape, size_t color) {
    this->shape = shape;
    this->color = color;
    pos = {5, 0};
  }
};

