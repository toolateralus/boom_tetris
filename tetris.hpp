#include "raylib.h"

#include <cstdlib>
#include <ctime>
#include <memory>
#include "rayui.hpp"
#include <stdio.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>
#pragma once

// an integer Vector2
#include <array>
#include <cstddef>
#include <vector>

constexpr size_t boardWidth = 10;
constexpr size_t boardHeight = 20;

#define BG_COLOR GetColor(0x12121212)

using namespace rayui;

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


struct HorizontalInput {
  HorizontalInput(bool left, bool right) : left(left), right(right) {}
  bool left, right;
};

// a grid cell.
struct Cell {
  size_t color = 0;
  bool empty = true;
};

struct Board {
  std::array<std::array<Cell, boardWidth>, boardHeight> rows = {};

  Cell &operator[](int x, int y) noexcept;
  
  Cell &get_cell(int x, int y) noexcept { return (*this)[x, y]; }
  
  auto begin() noexcept { return rows.begin(); }
  auto end() noexcept { return rows.end(); }

  // We need more information that just whether it collided or not: we need to
  // know what side we hit so we can depenetrate in the opposite direction.
  bool collides(Vec2 pos) noexcept;
};

// a group of cells the user is currently in control of.
struct Tetromino {
  size_t color = 0;
  Vec2 prev_position;
  Orientation prev_orientation;
  Vec2 position;
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
    position = {5, 0};
  }
};

struct Game;
struct BoardCell : Element {
  Game& game;
  Rectangle blockTxSourceRect;
  Cell& cell;
  virtual void draw(rayui::LayoutState &state) override;
  BoardCell(Position position, Game& game, Cell& cell, Rectangle blockTxSourceRect)
      : Element(position, {1,1}), game(game), cell(cell), blockTxSourceRect(blockTxSourceRect) {}
};

struct NumberText : Element {
  size_t* number;
  Color color;
  NumberText(Position position, Size size, size_t* number, Color color)
      : Element(position, size), number(number), color(color) {}
  virtual void draw(LayoutState &state) override;
};

struct Game {
  Grid createGrid();
  Grid gameGrid;
  void drawGame();
  int FindGamepad() const {
    for (int i = 0; i < 5; ++i) {
      if (IsGamepadAvailable(i)) 
        return i;
    }
    return -1;
  }
  
  // the play grid.
  Board board;
  
  // the upcoming shape & color of the next tetromino.
  Shape nextShape;
  int nextColor;

  // the piece the player is in control of.
  std::unique_ptr<Tetromino> tetromino;

  void reset();

  Game();
  ~Game();

  // TODO: make this more like classic tetris.
  static std::vector<float> gravityLevels;
  static std::unordered_map<Shape, std::vector<Vec2>> shapePatterns;
  // colors for each level[shape]
  static std::vector<std::vector<Color>> palette;

  // unit size of a cell on the grid, in pixels. based on resolution
  int blockSize = 32;
  
  // the block texture, used and tinted for every block.
  Texture2D blockTexture;
  // an index into the current pallette, based on level.
  size_t paletteIdx = 0;
  // at which rate are we moving the tetromino down?
  float gravity = 0.0f;
  // extra gravity for when the player is holding down.
  float playerGravity = 0.0f;
  size_t level = 0;
  size_t score = 0;
  size_t linesClearedThisLevel = 0;
  // are we in the main menu?
  bool inMenu = true;

  void setNextShapeAndColor();
  void processGameLogic();
  void drawUi();
  
  std::vector<size_t> checkLines();
  size_t clearLines(std::vector<size_t> &linesToClear);
  void adjustScoreAndLevel(size_t linesCleared);
  void saveTetromino();
  
  HorizontalInput delayedAutoShift();
  void cleanTetromino(std::unique_ptr<Tetromino> &tetromino);
  bool resolveCollision(std::unique_ptr<Tetromino> &tetromino);
  ShapeIndices getIndices(std::unique_ptr<Tetromino> &tetromino) const;
  std::shared_ptr<rayui::Grid> createBoardGrid();
};