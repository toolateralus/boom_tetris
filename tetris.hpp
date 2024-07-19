#include "raylib.h"

#include <algorithm>

#include <cstdlib>
#include <ctime>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <vector>
#pragma once

// an integer Vector2
#include <cstddef>
#include <vector>
struct Vec2 {
  int x, y;
};
// the shape of a tetromino, a group of cells.
enum struct Shape { L, J, Z, S, I, T, Square };
enum struct Orientation { Up, Right, Down, Left };


#define BG_COLOR GetColor(0x12121212)
// a basic color palette. only one palette item right now.

struct Tetromino;



struct Game {
  Shape *gNextShape = nullptr;
  size_t *gNextColor = nullptr;
  Tetromino *gTetromino = nullptr;
  
  const std::unordered_map<Shape, std::vector<Vec2>> gShapePatterns = {
      {Shape::L, {{0, -1}, {0, 0}, {0, 1}, {1, 1}}},
      {Shape::J, {{0, -1}, {0, 0}, {0, 1}, {-1, 1}}},
      {Shape::Z, {{0, -1}, {0, 0}, {1, 0}, {1, 1}}},
      {Shape::S, {{0, -1}, {0, 0}, {-1, 0}, {-1, 1}}},
      {Shape::I, {{0, -1}, {0, 0}, {0, 1}, {0, 2}}},
      {Shape::T, {{-1, 0}, {0, 0}, {1, 0}, {0, 1}}},
      {Shape::Square, {{0, 0}, {0, 1}, {1, 0}, {1, 1}}},
  };
  
  int gBlockSize = 32;
  const std::vector<std::vector<Color>> palette = {
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
};

static void setNextShapeAndColor(Game &game) {
	int num_shapes = (int)Shape::Square + 1;
	auto shape = Shape(rand() % num_shapes);
	*game.gNextShape = shape;
	*game.gNextColor = (size_t)std::min((int)shape, 4);
}

// a way to key into the grid to update a tetromino.
using ShapeIndices = std::vector<Vec2>;

// the direction of user input.
enum struct Direction { None, Left, Right, Down };

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
      return false;
    }
    if (!rows[y][x].empty) {
      return true;
    }
    return false;
  }
  
  void draw(Game &game) {
		auto blockTxSourceRect = Rectangle{0, 0, (float)game.blockTexture.width, (float)game.blockTexture.height};
    auto screen_width = GetScreenWidth();
    auto screen_height = GetScreenHeight();
    game.gBlockSize = std::min(screen_height / 20, screen_width / 26);
    auto halfScreen = screen_width / 2;
    auto leftStart = halfScreen - game.gBlockSize * 13;
    auto rightStart = halfScreen + game.gBlockSize * 5;
    DrawRectangle(leftStart, 0, game.gBlockSize * 26, game.gBlockSize * 20, DARKGRAY);
    // draw left side
    DrawText("Score:", leftStart + game.gBlockSize, 0, game.gBlockSize, WHITE);
    DrawText(std::to_string(game.score).c_str(), leftStart + game.gBlockSize, game.gBlockSize, game.gBlockSize, WHITE);
    DrawText("Level:", leftStart + game.gBlockSize, game.gBlockSize * 2, game.gBlockSize, WHITE);
    DrawText(std::to_string(game.level).c_str(), leftStart + game.gBlockSize, game.gBlockSize * 3, game.gBlockSize, WHITE);
    // draw right side
    DrawText("Next:", rightStart + game.gBlockSize, 0, game.gBlockSize, WHITE);
    DrawRectangle(rightStart + game.gBlockSize, game.gBlockSize, game.gBlockSize * 4, game.gBlockSize * 4, BLACK);
    auto nextBlockAreaCenterX = rightStart + game.gBlockSize * 2;
    auto nextBlockAreaCenterY = game.gBlockSize * 2;
    for (const auto& block : game.gShapePatterns.at(*game.gNextShape)) {
      auto color = game.palette[game.paletteIdx][*game.gNextColor];
      auto destX = nextBlockAreaCenterX + block.x * game.gBlockSize;
      auto destY = nextBlockAreaCenterY + block.y * game.gBlockSize;
      auto destRect = Rectangle{(float)destX, (float)destY, (float)game.gBlockSize, (float)game.gBlockSize};
      DrawTexturePro(game.blockTexture, blockTxSourceRect, destRect, {0,0}, 0, color);
    }
    // draw board
    auto boardStart = halfScreen - (game.gBlockSize * 10 / 2);
    size_t x = 0, y = 0;
    for (const auto &row : rows) {
      for (const auto &cell : row) {
        auto color = BLACK;
        if (!cell.empty) {
          color = game.palette[game.paletteIdx][cell.color];
					auto dx = x + boardStart;
					auto destRect = Rectangle{(float)dx, (float)y, (float)game.gBlockSize, (float)game.gBlockSize};
					DrawTexturePro(game.blockTexture, blockTxSourceRect, destRect, {0,0}, 0, color);
        } else {
					DrawRectangle(x + boardStart, y, game.gBlockSize, game.gBlockSize, color);
				}
				
        x += game.gBlockSize;
      }
      y += game.gBlockSize;
      x = 0;
    }
  }
  
  const std::vector<float> gGravityLevels = {
      1.0 / 48, 1.0 / 43, 1.0 / 38, 1.0 / 33, 1.0 / 28,
      1.0 / 23, 1.0 / 18, 1.0 / 13, 1.0 / 8,  1.0 / 6,
      1.0 / 6,  1.0 / 6,  1.0 / 6,  1.0 / 5,  1.0 / 5,
  };
  
  void checkLines(Game &game) {
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

    // todo: add more scoring, for soft & hard drops.
    auto level = game.level + 1;
    if (linesToBurn.size() == 1) {
      game.score += 40 * level;
    } else if (linesToBurn.size() == 2) {
      game.score += 100 * level;
    } else if (linesToBurn.size() == 3) {
      game.score += 300 * level;
    } else if (linesToBurn.size() == 4) {
      game.score += 1200 * level;
    }
    
    game.linesClearedThisLevel += linesToBurn.size();
    if (game.linesClearedThisLevel >= 10) {
      game.level++;
			if (game.level < gGravityLevels.size()) {
      	game.gravity += gGravityLevels[game.level];
			}
			game.linesClearedThisLevel = 0;
    }
  }
};

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
  void clean(Game &game, Board &gBoard) {
    auto indices = getIndices(game);
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
  bool resolveCollision(Game &game, Board &gBoard) {
    for (const auto idx : getIndices(game)) {
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

  ShapeIndices getIndices(Game &game) const {
    ShapeIndices indices;
    auto pattern = game.gShapePatterns.at(shape);
    for (const auto &idx : pattern) {
      const auto rotated = rotate(idx);
      indices.push_back({pos.x + rotated.x, pos.y + rotated.y});
    }
    return indices;
  }
  void saveState() {
    last_ori = orientation;
    last_pos = pos;
  }
  Tetromino(Game &game) {
		shape = *game.gNextShape;
		color = *game.gNextColor;
		setNextShapeAndColor(game);
    pos = {5, 0};
  }
};

struct HorizontalInput {
  HorizontalInput(bool left, bool right) : left(left), right(right) {}
  bool left, right;
};