#include "raylib.h"

#include <algorithm>

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <unordered_map>
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

// an integer Vector2
struct Vec2 {
  int x, y;
};
// the shape of a tetromino, a group of cells.
enum struct Shape { L, J, Z, S, I, T, Square };
enum struct Orientation { Up, Right, Down, Left };

Shape *gNextShape = nullptr;
size_t *gNextColor = nullptr;

// a static map of the coordinates of each shape in local space.
const std::unordered_map<Shape, std::vector<Vec2>> gShapePatterns = {
    {Shape::L, {{-1, 1}, {-1, 0}, {0, 0}, {1, 0}}},
    {Shape::J, {{-1, 0}, {0, 0}, {1, 0}, {1, 1}}},
    {Shape::Z, {{-1, 0}, {0, 0}, {0, 1}, {1, 1}}},
    {Shape::S, {{-1, 1}, {0, 1}, {0, 0}, {1, 0}}},
    {Shape::I, {{-1, 0}, {0, 0}, {1, 0}, {2, 0}}},
    {Shape::T, {{-1, 0}, {0, 0}, {1, 0}, {0, 1}}},
    {Shape::Square, {{0, 0}, {0, 1}, {1, 0}, {1, 1}}},
};

int gBlockSize = 32;
#define BG_COLOR GetColor(0x12121212)
// a basic color palette. only one palette item right now.
const std::vector<std::vector<Color>> gPalette = {
    {BLUE, LIME, YELLOW, ORANGE, RED}};


// a way to key into the grid to update a tetromino.
using ShapeIndices = std::vector<Vec2>;

// the block texture, used and tinted for every block.
Texture2D gTexture;

// an index into the current pallette, based on level.
size_t gCurrentPaletteIdx = 0;

// the direction of user input.
enum struct Direction { None, Left, Right, Down };

// at which rate are we moving the tetromino down?
float gGravity = 0.15f;
float gPlayerGravity = 0.0f;

size_t gLevel = 0;
size_t gScore = 0;
size_t gLinesClearedThisLevel = 0;


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

  void draw() {
		auto blockTxSourceRect = Rectangle{0, 0, (float)gTexture.width, (float)gTexture.height};
    auto screen_width = GetScreenWidth();
    auto screen_height = GetScreenHeight();
    gBlockSize = std::min(screen_height / 20, screen_width / 26);
    auto halfScreen = screen_width / 2;
    auto leftStart = halfScreen - gBlockSize * 13;
    auto rightStart = halfScreen + gBlockSize * 5;
    DrawRectangle(leftStart, 0, gBlockSize * 26, gBlockSize * 20, DARKGRAY);
    // draw left side
    DrawText("Score:", leftStart + gBlockSize, 0, gBlockSize, WHITE);
    DrawText(std::to_string(gScore).c_str(), leftStart + gBlockSize, gBlockSize, gBlockSize, WHITE);
    DrawText("Level:", leftStart + gBlockSize, gBlockSize * 2, gBlockSize, WHITE);
    DrawText(std::to_string(gLevel).c_str(), leftStart + gBlockSize, gBlockSize * 3, gBlockSize, WHITE);
    // draw right side
    DrawText("Next:", rightStart + gBlockSize, 0, gBlockSize, WHITE);
    DrawRectangle(rightStart + gBlockSize, gBlockSize, gBlockSize * 4, gBlockSize * 4, BLACK);
    auto nextBlockAreaCenterX = rightStart + gBlockSize * 2;
    auto nextBlockAreaCenterY = gBlockSize * 2;
    if (*gNextShape != Shape::I && *gNextShape != Shape::Square) {
      nextBlockAreaCenterX += gBlockSize / 2;
    }
    if (*gNextShape == Shape::I) {
      nextBlockAreaCenterY += gBlockSize / 2;
    }
    for (const auto& block : gShapePatterns.at(*gNextShape)) {
      auto color = gPalette[gCurrentPaletteIdx][*gNextColor];
      auto destX = nextBlockAreaCenterX + block.x * gBlockSize;
      auto destY = nextBlockAreaCenterY + block.y * gBlockSize;
      auto destRect = Rectangle{(float)destX, (float)destY, (float)gBlockSize, (float)gBlockSize};
      DrawTexturePro(gTexture, blockTxSourceRect, destRect, {0,0}, 0, color);
    }
    // draw board
    auto boardStart = halfScreen - (gBlockSize * 10 / 2);
    size_t x = 0, y = 0;
    for (const auto &row : rows) {
      for (const auto &cell : row) {
        auto color = BLACK;
        if (!cell.empty) {
          color = gPalette[gCurrentPaletteIdx][cell.color];
					auto dx = x + boardStart;
					auto destRect = Rectangle{(float)dx, (float)y, (float)gBlockSize, (float)gBlockSize};
					DrawTexturePro(gTexture, blockTxSourceRect, destRect, {0,0}, 0, color);
        } else {
					DrawRectangle(x + boardStart, y, gBlockSize, gBlockSize, color);
				}
				
        x += gBlockSize;
      }
      y += gBlockSize;
      x = 0;
    }
  }

  const std::vector<float> gGravityLevels = {
      1.0 / 48, 1.0 / 43, 1.0 / 38, 1.0 / 33, 1.0 / 28,
      1.0 / 23, 1.0 / 18, 1.0 / 13, 1.0 / 8,  1.0 / 6,
      1.0 / 6,  1.0 / 6,  1.0 / 6,  1.0 / 5,  1.0 / 5,
  };

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

    // todo: add more scoring, for soft & hard drops.
    auto level = gLevel + 1;
    if (linesToBurn.size() == 1) {
      gScore += 40 * level;
    } else if (linesToBurn.size() == 2) {
      gScore += 100 * level;
    } else if (linesToBurn.size() == 3) {
      gScore += 300 * level;
    } else if (linesToBurn.size() == 4) {
      gScore += 1200 * level;
    }

    gLinesClearedThisLevel += linesToBurn.size();
    if (gLinesClearedThisLevel >= 10) {
      gLevel++;
			if (gLevel < gGravityLevels.size()) {
      	gGravity += gGravityLevels[gLevel];
			}
			gLinesClearedThisLevel = 0;
    }
  }
};

Board gBoard = {};


void setNextShapeAndColor() {
	int num_shapes = (int)Shape::Square + 1;
	auto shape = Shape(rand() % num_shapes);
	*gNextShape = shape;
	*gNextColor = (size_t)std::min((int)shape, 4);
}

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

  ShapeIndices getIndices() const {
    ShapeIndices indices;
    auto pattern = gShapePatterns.at(shape);
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
  Tetromino() {
		shape = *gNextShape;
		color = *gNextColor;
		setNextShapeAndColor();
    pos = {5, 0};
  }
};

Tetromino *gTetromino = nullptr;

struct HorizontalInput {
  HorizontalInput(bool left, bool right) : left(left), right(right) {}
  bool left, right;
};

HorizontalInput DelayedAutoShift() {
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

  auto horizontal = DelayedAutoShift();

  if (IsKeyPressed(KEY_Z)) {
    gTetromino->saveState();
    gTetromino->orientation = gTetromino->getPreviousOrientation();
    gTetromino->resolveCollision();
  }
  if (IsKeyPressed(KEY_X) || IsKeyPressed(KEY_UP)) {
    gTetromino->saveState();
    gTetromino->orientation = gTetromino->getNextOrientation();
    gTetromino->resolveCollision();
  }
  if (horizontal.left) {
    gTetromino->saveState();
    gTetromino->pos.x--;
    gTetromino->resolveCollision();
  }
  if (horizontal.right) {
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
    gBoard.checkLines();
  }
}


int drawMenu() {
	ClearBackground(BLACK);
	static Texture2D texture = LoadTexture("res/title.png");
	
	auto source = Rectangle {0,0, (float)texture.width, (float)texture.height};
	auto dest = Rectangle {0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()};
	DrawTexturePro(texture, source, dest, {0,0}, 0, WHITE);
	
	auto x = GetScreenWidth() / 5, y = GetScreenHeight() / 2;
	DrawRectangle(0, y - 15, GetScreenWidth(), 60, BLACK);
	DrawText("Press a number 0-9 to start at that level.", x, y, 24, WHITE);
	for (int i = KEY_KP_0; i < KEY_KP_9; ++i) {
		if (IsKeyPressed(i)) {
			gLevel = i - KEY_KP_0;
			return true;	
		}
	}
	for (int i = KEY_ZERO; i < KEY_NINE + 1; ++i) {
		if (IsKeyPressed(i)) {
			gLevel = i - KEY_ZERO;
			return true;	
		}
	}
	return false;	
}

int main(int argc, char *argv[]) {
	gNextShape = new Shape;
	gNextColor = new size_t;
	setNextShapeAndColor();
	
  srand(time(0));
  
  InitWindow(800, 600, "boom taetris");
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  gTexture = LoadTexture("res/block.png");
  SetTargetFPS(30);

  // initialze board grid.
  for (int y = 0; y < 20; ++y) {
    auto &n = gBoard.emplace_back();
    for (int x = 0; x < 10; ++x) {
      n.emplace_back();
    }
  }
  
	bool menu_exited = false;
  while (!WindowShouldClose()) {
    BeginDrawing();
		
		if (!menu_exited) {
			menu_exited = drawMenu();
			EndDrawing();
			continue;
		}
		
    ClearBackground(BG_COLOR);
    processGameLogic();
    gBoard.draw();
    EndDrawing();
  }

  return 0;
}