#include "tetris.hpp"
#include <functional>
#include <raylib.h>
#include <string>

#include "rayui.hpp"

using namespace rayui;

Grid grid = {{24, 24}};

int drawMenu(Game &game) {
  ClearBackground(BLACK);
  static Texture2D texture = LoadTexture("res/title.png");
  
  auto source = Rectangle{0, 0, (float)texture.width, (float)texture.height};
  auto dest =
      Rectangle{0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()};
  DrawTexturePro(texture, source, dest, {0, 0}, 0, WHITE);
  auto state = LayoutState {PixelPosition{0, 0}, PixelSize{(float)GetScreenWidth(), (float)GetScreenHeight()}};
  grid.draw(state);
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

void setupMenuButtons(Game &game) {
  auto pos = Position{1,12};
  auto size = Size{2,2};
  grid.emplace_element<Rect>(Position{0, 11}, Size{1,4}, Style{GetColor(0x2b2b2baa), WHITE},LayoutKind::StretchHorizontal);
  for (int i = 0; i <= 9; ++i) {
    auto callback = std::function<void()>([&game, i]() {
      game.startLevel = i;
      game.level = i;
      game.inMenu = false;
    });
    grid.emplace_element<Button>(pos, size, std::to_string(i), callback);
    pos.x += size.width;
  }
  auto _40LineTxt = grid.emplace_element<Button>(pos, size, "40 lines", [&](){
    game.mode = GameMode::FortyLines;
    game.level = 5;
    game.startLevel = 5;
    game.inMenu = false;
  });
  _40LineTxt->fontSize = 16;
}

int main(int argc, char *argv[]) {
  srand(time(0));
  InitWindow(800, 600, "boom taetris");
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  SetTargetFPS(60);
  
  BeginDrawing();
  ClearBackground(BG_COLOR);
  DrawText("Loading Bagles & Cream Cheese\n", 124, 276, 36, WHITE);
  DrawText("      please wait...", 124, 300, 36, YELLOW);
  
  EndDrawing();
  if (WindowShouldClose()) {
    return 0;
  }
  
  Game game = Game();
  setupMenuButtons(game);
  
  while (!WindowShouldClose()) {
    BeginDrawing();
    
    if (game.inMenu) {
      drawMenu(game);
      
      if (!game.inMenu) {
        game.reset();
      }
      
      EndDrawing();
      continue;
    }
    
    //gamepadLogger(game);
    
    ClearBackground(BG_COLOR);
    game.processGameLogic();
    game.drawGame();
    EndDrawing();
  }
  
  game.scoreFile.write();
  
	
  return 0;
}