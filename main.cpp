#include "rayui.hpp"
#include "tetris.hpp"
#include <functional>
#include <raylib.h>
#include <string>

using namespace rayui;
using namespace boom_tetris;
Grid mainMenuGrid = {{24, 24}};
Grid gameOverGrid = {{24, 24}};

int drawMenu(Game &game) {
  ClearBackground(BLACK);
  static Texture2D texture = LoadTexture("res/title.png");
  // get full image
  auto imageWidth = (float)texture.width;
  auto imageHeight = (float)texture.height;
  auto source = Rectangle{0, 0, imageWidth, imageHeight};
  // scale image to height
  auto screenWidth = (float)GetScreenWidth();
  auto screenHeight = (float)GetScreenHeight();
  auto scale = screenHeight / imageHeight;
  auto destWidth = imageWidth * scale;
  // center image
  auto destX = (screenWidth - destWidth) / 2;
  auto dest = Rectangle{destX, 0, destWidth, screenHeight};
  DrawTexturePro(texture, source, dest, {0, 0}, 0, WHITE);
  auto state =
      LayoutState{PixelPosition{0, 0},
                  PixelSize{screenWidth, screenHeight}};
  mainMenuGrid.draw(state);
  return true;
}

// TODO: figure out why, even though we get a valid gamepad 0, we never can
// query buttons properly.
void gamepadLogger(Game &game) {
  auto gamepad = game.findGamepad();
  system("clear");
  printf("gamepad: %d:\n", gamepad);
  for (int i = GAMEPAD_BUTTON_LEFT_FACE_UP; i <= GAMEPAD_BUTTON_RIGHT_THUMB;
       ++i) {
    printf("is_down: %d\n", IsGamepadButtonDown(gamepad, i));
  }
}

void setupMenuButtons(Game &game) {
  auto pos = Position{1, 21};
  auto size = Size{2, 2};
  mainMenuGrid.emplace_element<Rect>(Position{0, 23}, Size{1, 4},
                                     Style{GetColor(0x2b2b2baa), WHITE},
                                     LayoutKind::StretchHorizontal);
  for (int i = 0; i <= 9; ++i) {
    auto callback = std::function<void()>([&game, i]() {
      game.reset();
      game.startLevel = i;
      game.level = i;
      game.scene = Game::Scene::InGame;
      game.gravity = game.gravityLevels[i];
    });
    mainMenuGrid.emplace_element<Button>(pos, size, std::to_string(i),
                                         callback);
    pos.x += size.width;
  }
  auto _40LineTxt =
      mainMenuGrid.emplace_element<Button>(pos, size, "40 lines", [&]() {
        game.reset();
        game.mode = Game::Mode::FortyLines;
        game.level = 5;
        game.startLevel = 5;
        game.scene = Game::Scene::InGame;
        game.gravity = game.gravityLevels[game.level];
      });
  _40LineTxt->fontSize = 16;
}

void setupGameOverMenu(Game &game) {
  auto size = Size{2, 2};
  
  gameOverGrid.emplace_element<Rect>(Position{3, 3}, Size{18, 18}, Style{GetColor(0x2b2b2bcc), WHITE});
  
  auto label = gameOverGrid.emplace_element<Label>(Position(8, 5), Size(2, 2),
                                                    "Game Over", RED);
                                                   
  gameOverGrid.emplace_element<Label>(Position{9, 8}, Size{1,1}, "Score:", WHITE);
  gameOverGrid.emplace_element<NumberText>(Position{11, 9}, Size{1,1}, &game.score, MAGENTA);
  
  gameOverGrid.emplace_element<Label>(Position{9, 11}, Size{1,1}, "Time:", WHITE);
  gameOverGrid.emplace_element<TimeText>(Position{11, 12}, Size{1,1}, &game.elapsed, GREEN);
  
  gameOverGrid.emplace_element<Label>(Position{9, 14}, Size{1,1}, "Lines:", WHITE);
  gameOverGrid.emplace_element<NumberText>(Position{11, 15}, Size{1,1}, &game.totalLinesCleared, ORANGE);
  
  gameOverGrid.emplace_element<Button>(Position{7, 17}, Size{5,2}, "Main Menu", [&](){
    game.scene = Game::Scene::MainMenu;
  });
  gameOverGrid.emplace_element<Button>(Position{13, 17}, Size{5,2}, "Retry", [&](){
    game.reset();
    game.scene = Game::Scene::InGame;
  });
  
  
}
int main(int argc, char *argv[]) {
  srand(time(0));
  InitWindow(800, 600, "boom taetris");
  InitAudioDevice();
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
    ClearBackground(BG_COLOR);
    switch (game.scene) {
      case Game::Scene::MainMenu: {
        drawMenu(game);
        break;
      }
      case Game::Scene::GameOver: {
        game.drawGame();
        auto state =
            LayoutState{0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()};
        gameOverGrid.draw(state);
        break;
      }
      case Game::Scene::InGame: {
        // gamepadLogger(game);
        game.processGameLogic();
        game.drawGame();
        break;
      }
    }
    EndDrawing();
  }

  game.scoreFile.write();
  CloseAudioDevice();
  return 0;
}