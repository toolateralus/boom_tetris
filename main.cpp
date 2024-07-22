#include "rayui.hpp"
#include "tetris.hpp"
#include <functional>
#include <raylib.h>
#include <string>

using namespace rayui;
using namespace boom_tetris;

struct UI {
  Grid titleGrid = {{23,23}};
  Grid mainMenuGrid = {{23, 23}};
  Grid gameOverGrid = {{24, 24}};
  Grid settingsGrid = {{23, 23}};
  Style buttonStyle = Style{BLACK, WHITE, BLACK, 3};
  
  enum struct Menu {
    Title,
    Main,
    Settings,
    GameOver,
  } menu = Menu::Title;
  
  int drawMenu(Game &game) {
    ClearBackground(BLACK);
    LayoutState state = {{0, 0},
                         {(float)GetScreenWidth(), (float)GetScreenHeight()}};
    switch (menu) {
    case Menu::Title: {
      titleGrid.draw(state);
    } break;
    case Menu::Main: {
      mainMenuGrid.draw(state);
    } break;
    case Menu::Settings: {
      settingsGrid.draw(state);
    } break;
    case Menu::GameOver: {
      game.drawGame();
      auto state =
          LayoutState{0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()};
      gameOverGrid.draw(state);
    } break;
    }

    return true;
  }
  
  UI(Game &game) {
    setupMainMenu(game);
    setupGameOver(game);
    setupTitleMenu();
    setupSettingsMenu(game);
  }
  
  
  void setupSettingsMenu(Game &game) {
    
    auto image = settingsGrid.emplace_element<rayui::Image>(
        Position{0, 0}, settingsGrid.subdivisions,
        std::string("res/title.png"));
    image->fillType = rayui::FillType::FillVertical;
    image->hAlignment = HAlignment::Center;
    
    
    settingsGrid.emplace_element<Rect>(Position{6, 15}, Size{11, 6},
                                       Style{GetColor(0x1b1b1bcc), WHITE},
                                       LayoutKind::None);
    
    
    *game.volumeLabel = "Volume: 100";
    auto volumeSlider = settingsGrid.emplace_element<Slider>(
        Position{7, 17}, Size{3, 2}, game.volumeLabel, 0, 100, 100,
        [&](float volume) {
          *game.volumeLabel = "Volume: " + std::to_string((int)volume);
          SetMasterVolume(volume / 100.0f);
        });
      
    volumeSlider->style.background = MAGENTA;
    volumeSlider->style.foreground = WHITE;
    volumeSlider->fontSize = 24;
    
    auto pos = Position{9, 18};
    auto btn= settingsGrid.emplace_element<Button>(pos, Size{5,2}, "Back", [this](){
      this->menu = Menu::Title;
    }, buttonStyle);
    
  }
  void setupTitleMenu() {
    auto image = titleGrid.emplace_element<rayui::Image>(
        Position{0, 0}, titleGrid.subdivisions,
        std::string("res/title.png"));
    image->fillType = rayui::FillType::FillVertical;
    image->hAlignment = HAlignment::Center;
    
    auto pos = Position{9, 18};
    auto btn= titleGrid.emplace_element<Button>(pos, Size{5,2}, "Play", [this](){
      this->menu = Menu::Main;
    }, buttonStyle);
    
    btn->margin = {3,3,3,3};
    
    pos.y += 2;
    auto btn1= titleGrid.emplace_element<Button>(pos, Size{5,2}, "Settings", [this](){
      this->menu = Menu::Settings;
    }, buttonStyle);
    
    btn1->margin = {3,3,3,3};
  }
  void setupMainMenu(Game &game) {
    auto pos = Position{1, 21};
    auto size = Size{2, 2};
    auto image = mainMenuGrid.emplace_element<rayui::Image>(
        Position{0, 0}, mainMenuGrid.subdivisions,
        std::string("res/title.png"));
    image->fillType = rayui::FillType::FillVertical;
    image->hAlignment = HAlignment::Center;
    
    mainMenuGrid.emplace_element<Rect>(Position{0, 19}, Size{1, 4},
                                       Style{GetColor(0x1b1b1bcc), WHITE},
                                       LayoutKind::StretchHorizontal);
    
    auto label = mainMenuGrid.emplace_element<Label>(
        Position{3, 19}, Size{1, 1}, "Level:", WHITE);
    
    mainMenuGrid.emplace_element<Label>(
        Position{17, 19}, Size{1, 1}, "Other Modes:", WHITE);
    
    
    auto j = 0;
    for (int i = 0; i <= 9; ++i) {
      auto btnPos = Position{24 / 3 - 2 + i * size.width, pos.y - 2};
      auto callback = std::function<void()>([&game, i]() {
        game.reset();
        game.startLevel = i;
        game.level = i;
        game.scene = Game::Scene::InGame;
        game.gravity = game.gravityLevels[i];
      });
      if (i > 4) {
        btnPos.y += 2;
        btnPos.x = 24 / 3 - 2 + j;
        j += size.width;
        ;
      }
      auto button = mainMenuGrid.emplace_element<Button>(
          btnPos, size, std::to_string(i), callback, buttonStyle);
      pos.x += size.width;
      button->margin = {3, 3, 3, 3};
    }
    
    auto backButton = mainMenuGrid.emplace_element<Button>(Position{0, 20}, Size{2,2}, "Back", [&](){
      menu = Menu::Title;
    }, buttonStyle);
    
    auto fortyLineBtn =
        mainMenuGrid.emplace_element<Button>(Position{20,20}, Size{2,2}, "40 lines", [&]() {
          game.mode = Game::Mode::FortyLines;
          game.reset();
          game.mode = Game::Mode::FortyLines;
          game.level = 5;
          game.startLevel = 5;
          game.scene = Game::Scene::InGame;
          game.gravity = game.gravityLevels[game.level];
        }, buttonStyle) ;
    fortyLineBtn->fontSize = 18;
  }

  void setupGameOver(Game &game) {

    gameOverGrid.emplace_element<Rect>(Position{3, 3}, Size{18, 18},
                                       Style{GetColor(0x2b2b2bcc), WHITE});

    auto label = gameOverGrid.emplace_element<Label>(Position(8, 5), Size(2, 2),
                                                     "Game Over", RED);

    gameOverGrid.emplace_element<Label>(Position{9, 8}, Size{1, 1},
                                        "Score:", WHITE);
    gameOverGrid.emplace_element<NumberText>(Position{11, 9}, Size{1, 1},
                                             &game.score, MAGENTA);

    gameOverGrid.emplace_element<Label>(Position{9, 11}, Size{1, 1},
                                        "Time:", WHITE);
    gameOverGrid.emplace_element<TimeText>(Position{11, 12}, Size{1, 1},
                                           &game.elapsed, GREEN);

    gameOverGrid.emplace_element<Label>(Position{15, 11}, Size{1, 1},
                                        "40 Line Best", WHITE);

    gameOverGrid.emplace_element<TimeText>(Position{16, 12}, Size{1, 1},
                                           &game.scoreFile.fortyLinesPb, GREEN);

    gameOverGrid.emplace_element<Label>(Position{9, 14}, Size{1, 1},
                                        "Lines:", WHITE);

    gameOverGrid.emplace_element<NumberText>(Position{11, 15}, Size{1, 1},
                                             &game.totalLinesCleared, ORANGE);

    gameOverGrid.emplace_element<Button>(
        Position{7, 17}, Size{5, 2}, "Main Menu",
        [&]() { game.scene = Game::Scene::MainMenu; });

    gameOverGrid.emplace_element<Button>(Position{13, 17}, Size{5, 2}, "Retry",
                                         [&]() {
                                           game.reset();
                                           game.scene = Game::Scene::InGame;
                                         });
  }
};

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
  UI ui = UI(game);
  
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BG_COLOR);
    switch (game.scene) {
    case Game::Scene::MainMenu: {
      ui.drawMenu(game);
      break;
    }
    case Game::Scene::GameOver: {
     
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