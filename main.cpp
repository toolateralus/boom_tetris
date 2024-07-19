#include "tetris.hpp"




int drawMenu(Game &game) {
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
  Game game = {};
  
	game.gNextShape = new Shape;
	game.gNextColor = new size_t;
	game.setNextShapeAndColor();
	
  srand(time(0));
  
  InitWindow(800, 600, "boom taetris");
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  game.blockTexture = LoadTexture("res/block.png");
  SetTargetFPS(30);
  
	game.inMenu = true;
  while (!WindowShouldClose()) {
    BeginDrawing();
	
		if (!game.inMenu) {
			game.inMenu = drawMenu(game);
			EndDrawing();
			continue;
		}
    ClearBackground(BG_COLOR);
    game.processGameLogic();
    game.drawUi();
    game.draw();
    EndDrawing();
  }

  return 0;
}