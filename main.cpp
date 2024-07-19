#include "tetris.hpp"
#include <cmath>

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


Board gBoard;
Game game = {};



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
bool gGameDone = false;
void processGameLogic() {

  if (game.gTetromino == nullptr) {
    game.gTetromino = new Tetromino(game);
    game.gTetromino->saveState();
    if (game.gTetromino->resolveCollision(game, gBoard)) {
      printf("game done\n");
      gGameDone = true;
      return;
    }
  }
  
  // DEBUG BUTTON: restart current piece.
  if (IsKeyPressed(KEY_R)) {
    if (game.gTetromino) {
      game.gTetromino->clean(game, gBoard);
      delete game.gTetromino;
    }
    game.gTetromino = new Tetromino(game);
  }
  
  static float budge = 0.0;
  game.gTetromino->clean(game, gBoard);

  auto horizontal = DelayedAutoShift();

  if (IsKeyPressed(KEY_Z)) {
    game.gTetromino->saveState();
    game.gTetromino->orientation = game.gTetromino->getPreviousOrientation();
    game.gTetromino->resolveCollision(game, gBoard);
  }
  if (IsKeyPressed(KEY_X) || IsKeyPressed(KEY_UP)) {
    game.gTetromino->saveState();
    game.gTetromino->orientation = game.gTetromino->getNextOrientation();
    game.gTetromino->resolveCollision(game, gBoard);
  }
  if (horizontal.left) {
    game.gTetromino->saveState();
    game.gTetromino->pos.x--;
    game.gTetromino->resolveCollision(game, gBoard);
  }
  if (horizontal.right) {
    game.gTetromino->saveState();
    game.gTetromino->pos.x++;
    game.gTetromino->resolveCollision(game, gBoard);
  }
  if (IsKeyDown(KEY_DOWN)) {
    game.playerGravity = 0.25f;
  }

  game.gTetromino->saveState();
  budge += game.gravity + game.playerGravity;
  game.playerGravity = 0.0f;
  auto floored = std::floor(budge);
  if (floored > 0) {
    game.gTetromino->pos.y += 1;
    budge = 0;
  }

  auto hit_bottom = game.gTetromino->resolveCollision(game, gBoard);
  for (const auto &idx : game.gTetromino->getIndices(game)) {
    if (idx.x < 0 || idx.x >= 10 || idx.y < 0 || idx.y >= 20) {
      continue;
    }
    auto &cell = gBoard[idx.x, idx.y];
    cell.empty = false;
    cell.color = game.gTetromino->color;
  }

  if (hit_bottom) {
    delete game.gTetromino;
    game.gTetromino = nullptr;
    gBoard.checkLines(game);
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
			game.level = i - KEY_KP_0;
			return true;	
		}
	}
	for (int i = KEY_ZERO; i < KEY_NINE + 1; ++i) {
		if (IsKeyPressed(i)) {
			game.level = i - KEY_ZERO;
			return true;	
		}
	}
	return false;	
}

int main(int argc, char *argv[]) {
	game.gNextShape = new Shape;
	game.gNextColor = new size_t;
	setNextShapeAndColor(game);
	
  srand(time(0));
  
  InitWindow(800, 600, "boom taetris");
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  game.blockTexture = LoadTexture("res/block.png");
  SetTargetFPS(30);
  
  // initialze board grid.
  for (int y = 0; y < 20; ++y) {
    auto &n = gBoard.emplace_back();
    for (int x = 0; x < 10; ++x) {
      n.emplace_back();
    }
  }
  
	game.inMenu = true;
  while (!WindowShouldClose()) {
    BeginDrawing();
		if (gGameDone) {
      gGameDone = false;
      game.inMenu = false;
    }
		if (!game.inMenu) {
			game.inMenu = drawMenu();
			EndDrawing();
			continue;
		}
		
    ClearBackground(BG_COLOR);
    processGameLogic();
    gBoard.draw(game);
    EndDrawing();
  }

  return 0;
}