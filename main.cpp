#include "rayui.hpp"
#include "tetris.hpp"
#include <functional>
#include <raylib.h>
#include <string>

using namespace rayui;

Grid mainMenuGrid = {{24, 24}};
Grid gameOverGrid = {{24, 24}};

int drawMenu(Game &game) {
  ClearBackground(BLACK);
  static Texture2D texture = LoadTexture("res/title.png");

  auto source = Rectangle{0, 0, (float)texture.width, (float)texture.height};
  auto dest =
      Rectangle{0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()};
  DrawTexturePro(texture, source, dest, {0, 0}, 0, WHITE);
  auto state =
      LayoutState{PixelPosition{0, 0},
                  PixelSize{(float)GetScreenWidth(), (float)GetScreenHeight()}};
  mainMenuGrid.draw(state);
  return true;
}

// TODO: figure out why, even though we get a valid gamepad 0, we never can
// query buttons properly.
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
  auto pos = Position{1, 12};
  auto size = Size{2, 2};
  mainMenuGrid.emplace_element<Rect>(Position{0, 11}, Size{1, 4},
                                     Style{GetColor(0x2b2b2baa), WHITE},
                                     LayoutKind::StretchHorizontal);
  for (int i = 0; i <= 9; ++i) {
    auto callback = std::function<void()>([&game, i]() {
      game.reset();
      game.startLevel = i;
      game.level = i;
      game.scene = Game::Scene::InGame;
    });
    mainMenuGrid.emplace_element<Button>(pos, size, std::to_string(i),
                                         callback);
    pos.x += size.width;
  }
  auto _40LineTxt =
      mainMenuGrid.emplace_element<Button>(pos, size, "40 lines", [&]() {
        game.reset();
        game.mode = GameMode::FortyLines;
        game.level = 5;
        game.startLevel = 5;
        game.scene = Game::Scene::InGame;
      });
  _40LineTxt->fontSize = 16;
}

void setupGameOverMenu(Game &game) {
  auto size = Size{2, 2};
  
  gameOverGrid.emplace_element<Rect>(Position{4, 4}, Size{18, 16}, Style{GetColor(0x2b2b2bcc), WHITE});
  
  auto label = gameOverGrid.emplace_element<Label>(Position(9, 5), Size(2, 2),
                                                    "Game Over", RED);
                                                   
  gameOverGrid.emplace_element<Label>(Position{10, 8}, Size{1,1}, "Score:", WHITE);
  gameOverGrid.emplace_element<NumberText>(Position{11, 9}, Size{1,1}, &game.score, MAGENTA);
  
  gameOverGrid.emplace_element<Label>(Position{10, 11}, Size{1,1}, "Time:", WHITE);
  gameOverGrid.emplace_element<TimerText>(Position{11, 12}, Size{1,1}, &game.elapsed, GREEN);
  
  gameOverGrid.emplace_element<Label>(Position{10, 14}, Size{1,1}, "Lines:", WHITE);
  gameOverGrid.emplace_element<NumberText>(Position{11, 15}, Size{1,1}, &game.totalLinesCleared, ORANGE);
  
  gameOverGrid.emplace_element<Button>(Position{8, 17}, Size{5,2}, "Main Menu", [&](){
    game.scene = Game::Scene::MainMenu;
  });
  gameOverGrid.emplace_element<Button>(Position{14, 17}, Size{5,2}, "Retry", [&](){
    game.reset();
    game.scene = Game::Scene::InGame;
  });
  
  
}
int main(int argc, char *argv[]) {
  srand(time(0));
  InitWindow(800, 600, "boom taetris");
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  SetTargetFPS(60);
  
  BeginDrawing();
  ClearBackground(BG_COLOR);
  DrawText("Loading Bagles & Cream Cheese\n", 124, 276, 36, WHITE);
  DrawText("      please wait...", 124, 300, 36, GREEN);
  
  EndDrawing();
  if (WindowShouldClose()) {
    return 0;
  }

  Game game = Game();
  setupMenuButtons(game);
  setupGameOverMenu(game);

  while (!WindowShouldClose()) {
    BeginDrawing();
    switch (game.scene) {
      case Game::Scene::MainMenu: {
        drawMenu(game);
        break;
      }
      case Game::Scene::GameOver: {
        ClearBackground(BG_COLOR);
        auto state =
            LayoutState{0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()};
        gameOverGrid.draw(state);
        break;
      }
      case Game::Scene::InGame: {
        // gamepadLogger(game);
        ClearBackground(BG_COLOR);
        game.processGameLogic();
        game.drawGame();
        break;
      }
    }
    EndDrawing();
  }

  game.scoreFile.write();

  return 0;
}