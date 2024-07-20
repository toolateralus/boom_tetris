#include "raylib.h"

#include "rayui.hpp"
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
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
enum struct Shape { L, J, Z, S, I, T, O };
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

#include <cstdlib> // For getenv
#include <fstream>
#include <string>
#ifdef _WIN32
#include <windows.h> // For GetEnvironmentVariable
#endif

struct ScoreFile {
  size_t high_score = 0;

  static std::string getScoreFilePath() {
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
  
  static void createDirectoryAndFile(const std::string &path) {
    try {
      std::filesystem::path dirPath = std::filesystem::path(path).parent_path();
      if (!std::filesystem::exists(dirPath)) {
        bool created = std::filesystem::create_directories(dirPath);
        if (created) {
          std::cout << "Directory created successfully: " << dirPath
                    << std::endl;
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

  void read() {
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
  void write() {
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
struct PieceViewer : Element {
  Game& game;
  virtual void draw(rayui::LayoutState &state) override;
  PieceViewer(Position position, Size size, Game& game)
      : Element(position, size), game(game) {}
};
struct BoardCell : Element {
  Game& game;
  Cell& cell;
  virtual void draw(rayui::LayoutState &state) override;
  BoardCell(Position position, Game& game, Cell& cell)
      : Element(position, {1,1}), game(game), cell(cell) {}
};

struct NumberText : Element {
  size_t *number;
  Color color;
  NumberText(Position position, Size size, size_t *number, Color color)
      : Element(position, size), number(number), color(color) {}
  virtual void draw(LayoutState &state) override;
};

struct Game {
  ScoreFile scoreFile;

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

  Rectangle blockTxSourceRect;
  
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
  std::vector<float> gravityLevels;
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
  size_t startLevel;
  size_t linesClearedThisLevel = 0;
  size_t totalLinesCleared = 0;
  // are we in the main menu?
  bool inMenu = true;
  
  void generateGravityLevels(int totalLevels) {
    float divisor = 48.0;
    for (int level = 0; level < totalLevels; ++level) {
        gravityLevels.push_back(1.0 / divisor);
        if (level < 10) {
          divisor -= 5.0;
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
    }
  }
  
  
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