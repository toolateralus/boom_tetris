#include "tetris.hpp"
#include <raylib.h>

int drawMenu(Game &game) {
  ClearBackground(BLACK);
  static Texture2D texture = LoadTexture("res/title.png");

  auto source = Rectangle{0, 0, (float)texture.width, (float)texture.height};
  auto dest =
      Rectangle{0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()};
  DrawTexturePro(texture, source, dest, {0, 0}, 0, WHITE);

  auto x = GetScreenWidth() / 5, y = GetScreenHeight() / 2;
  DrawRectangle(0, y - 15, GetScreenWidth(), 60, BLACK);
  DrawText("Press a number 0-9 to start at that level.", x, y, 24, WHITE);
  for (int i = KEY_KP_0; i < KEY_KP_9; ++i) {
    if (IsKeyPressed(i)) {
      game.level = i - KEY_KP_0;
      return false;
    }
  }
  for (int i = KEY_ZERO; i < KEY_NINE + 1; ++i) {
    if (IsKeyPressed(i)) {
      game.level = i - KEY_ZERO;
      return false;
    }
  }
  return true;
}

// TODO: figure out why, even though we get a valid gamepad 0, we never can query buttons properly.
void gamepadLogger(Game &game) {
  auto gamepad = game.FindGamepad();
  system("clear");
  printf("gamepad: %d:\n", gamepad);
  for (int i = GAMEPAD_BUTTON_LEFT_FACE_UP; i <= GAMEPAD_BUTTON_RIGHT_THUMB;
       ++i) {
    printf("is_down: %d\n", IsGamepadButtonDown(gamepad, i));
  }
}
int main(int argc, char *argv[]) {

  srand(time(0));
  InitWindow(800, 600, "boom taetris");
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  SetTargetFPS(30);

  Game game = Game();
  while (!WindowShouldClose()) {
    BeginDrawing();

    if (game.inMenu) {
      game.inMenu = drawMenu(game);

      // if we've exited the menu, reset the game state, for when we game over
      // and re-enter the game.
      if (!game.inMenu) {
        game.reset();
      }
      
      EndDrawing();
      continue;
    }
    
    //gamepadLogger(game);
    
    ClearBackground(BG_COLOR);
    game.processGameLogic();
    game.drawUi();
    game.draw();
    EndDrawing();
  }
  
	
  return 0;
}