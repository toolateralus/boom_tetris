#include "tetris.hpp"
#include <raylib.h>

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
  auto pos = Position{2,12};
  auto size = Size{2,2};
  
  auto zero = std::function<void()>([&game](){
    game.level = 0;
    game.inMenu = false;
  });
  auto one = std::function<void()>([&game](){
    game.level = 1;
    game.inMenu = false;
  });
  auto two = std::function<void()>([&game](){
    game.level = 2;
    game.inMenu = false;
  });
  auto three = std::function<void()>([&game](){
    game.level = 3;
    game.inMenu = false;
  });
  auto four = std::function<void()>([&game](){
    game.level = 4;
    game.inMenu = false;
  });
  auto five = std::function<void()>([&game](){
    game.level = 5;
    game.inMenu = false;
  });
  auto six = std::function<void()>([&game](){
    game.level = 6;
    game.inMenu = false;
  });
  auto seven = std::function<void()>([&game](){
    game.level = 7;
    game.inMenu = false;
  });
  auto eight = std::function<void()>([&game](){
    game.level = 8;
    game.inMenu = false;
  });
  auto nine = std::function<void()>([&game](){
    game.level = 9;
    game.inMenu = false;
  });
  grid.emplace_element<Rect>(Position{0, 11}, Size{1,4}, Style{GetColor(0x2b2b2baa), WHITE},LayoutKind::StretchHorizontal);
  grid.emplace_element<Button>(pos, size, (char*)"0", zero);
  pos.x += size.width;
  grid.emplace_element<Button>(pos, size, (char*)"1", one);
  pos.x += size.width;
  grid.emplace_element<Button>(pos, size, (char*)"2", two);
  pos.x += size.width;
  grid.emplace_element<Button>(pos, size, (char*)"3", three);
  pos.x += size.width;
  grid.emplace_element<Button>(pos, size, (char*)"4", four);
  pos.x += size.width;
  grid.emplace_element<Button>(pos, size, (char*)"5", five);
  pos.x += size.width;
  grid.emplace_element<Button>(pos, size, (char*)"6", six);
  pos.x += size.width;
  grid.emplace_element<Button>(pos, size, (char*)"7", seven);
  pos.x += size.width;
  grid.emplace_element<Button>(pos, size, (char*)"8", eight);
  pos.x += size.width;
  grid.emplace_element<Button>(pos, size, (char*)"9", nine);
}

int main(int argc, char *argv[]) {
  
  
  
  srand(time(0));
  InitWindow(800, 600, "boom taetris");
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  SetTargetFPS(30);
  
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
    game.drawUi();
    game.draw();
    EndDrawing();
  }
  
	
  return 0;
}