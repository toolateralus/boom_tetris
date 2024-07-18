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
  std::vector<std::vector<Cell>> array;
  Cell &operator[](int x, int y) {
    if (array.size() > y) {
      auto &inner = array[y];
      if (inner.size() > x) {
        return inner[x];
      }
    }
    throw std::runtime_error(
        "invalid indices into Board: (x=" + std::to_string(x) +
        ", y=" + std::to_string(y) + ")");
  }

  auto begin() { return array.begin(); }
  auto end() { return array.end(); }

  template <typename... Args> auto &emplace_back(Args &&...args) {
    return array.emplace_back(args...);
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

// an integer Vector2
struct Vec2 {
  size_t x, y;
};

// a way to key into the grid to update a tetromino.
using ShapeIndices = std::vector<Vec2>;

// a group of cells the user is currently in control of.
struct Tetromino {
  size_t color = 0;
  Vec2 pos;
  Shape shape;

  // discard old data. more specifically, write empty to all the cells that we
  // currently exist in, so that we can safely move into new cells.
  void clean() {
    auto indices = getIndices();
    for (const auto &idx : indices) {
			if (idx.y < 0 ||idx.y >= 20 || idx.x < 0 || idx.x >= 10) {
				continue;
			}
      auto &cell = gBoard[idx.x, idx.y];
      cell.empty = true;
      cell.color = 0;
    }
  }
  
	// returns false if the piece hasn't hit the bottom of the board,
	// true if it has.
	bool resolveCollision() {
		bool hitBottom = false;
		bool hitWallLeft = false;
		bool hitWallRight = false;
		
    for (const auto idx : getIndices()) {
      if (idx.y >= 20) {
        hitBottom = true;
      }
			if (idx.x < 0) {
				hitWallLeft = true;
			}
			if (idx.x >= 10) {
				hitWallRight = true;
			}
    }
    
		// if we hit the bottom, we've not resolved the Y collision
		bool resolved_y = !hitBottom;
		
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
		
		// if we've hit either of the walls, we need to do a horizontal collision resolution.
		bool resolved_x = !(hitWallLeft || hitWallRight);
		
		while (!resolved_x) {
			if (hitWallLeft) {
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
		return hitBottom;
	}
	
  bool move(Direction dir) {
    static float budge = 0.0;
    
    clean();
    
    switch (dir) {
    case Direction::Left:
      pos.x--;
      break;
    case Direction::Right:
      pos.x++;
      break;
    case Direction::Down:
      gPlayerGravity = 0.25f;
    case Direction::None:
      budge += gGravity + gPlayerGravity;
			gPlayerGravity = 0.0f;
      auto floored = std::floor(budge);
      if (floored > 0) {
        pos.y += 1;
        budge = 0;
      }

      break;
    }
		
		bool hitBottom = resolveCollision();
	
		// now we are completely in bounds, we draw our cells.
		for (const auto &idx: getIndices()) {
      auto &cell = gBoard[idx.x, idx.y];
      cell.empty = false;
      cell.color = color;
		}
		
    return hitBottom;
  }

  ShapeIndices getIndices() const {
    ShapeIndices indices;
    auto pattern = gShapePatterns.at(shape);
    for (const auto &[dx, dy] : pattern) {
      indices.push_back({pos.x + dx, pos.y + dy});
    }
    return indices;
  }

  static Tetromino spawn(size_t x, size_t y) {
    int num_shapes = (int)Shape::T + 1;
    Shape rand_shape = Shape(rand() % num_shapes);
    auto color = (size_t)std::min((int)rand_shape, 4);
    return Tetromino{.color = color, .pos = {x, y}, .shape = rand_shape};
  }
};

Tetromino *gTetromino = nullptr;

void drawBoard() {
  static auto halfScreen = GetScreenWidth() / 2;
  static auto boardStart = halfScreen - (UNIT * 10 / 2);
  size_t x = 0, y = 0;

  for (const auto &row : gBoard) {
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

void processGameLogic() {

  if (gTetromino == nullptr) {
    gTetromino = new Tetromino(Tetromino::spawn(5, 0));
  }

  // DEBUG BUTTON: restart current piece.
  if (IsKeyPressed(KEY_R)) {
    if (gTetromino) {
      gTetromino->clean();
      delete gTetromino;
    }
    gTetromino = new Tetromino(Tetromino::spawn(5, 0));
  }
  
	auto hit_bottom = false;
	
  if (IsKeyDown(KEY_LEFT)) {
    hit_bottom = gTetromino->move(Direction::Left);
  } else if (IsKeyDown(KEY_RIGHT)) {
    hit_bottom = gTetromino->move(Direction::Right);
  } else if (IsKeyDown(KEY_DOWN)) {
    hit_bottom = gTetromino->move(Direction::Down);
  } else {
		hit_bottom = gTetromino->move(Direction::None);
	}
	
	if (hit_bottom)  {
		delete gTetromino;
		gTetromino = nullptr;
	}
	
}
int main(int argc, char *argv[]) {
  
  InitWindow(800, 600, "boom taetris");
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
    drawBoard();
    EndDrawing();
  }

  return 0;
}