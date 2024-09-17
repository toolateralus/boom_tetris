#include "rayui.hpp"
#include "tetris.hpp"
#include <cmath>
#include <cstddef>
#include <functional>
#include <raylib.h>
#include <string>

using namespace rayui;
using namespace boom_tetris;

struct UI {
  Grid titleGrid = {{23, 23}};
  Grid mainMenuGrid = {{23, 23}};
  Grid gameOverGrid = {{24, 24}};
  Grid settingsGrid = {{23, 23}};
  Style buttonStyle = Style{BLACK, WHITE, BLACK, 3};

  std::vector<std::shared_ptr<Button>> levelButtons;

  bool shiftModifier = false;


  Texture2D titleImage = LoadTexture("res/title.png");
  std::vector<Texture2D> titleAnimationFrames = {};

  enum struct Menu {
    Title,
    Main,
    Settings,
    GameOver,
  } menu = Menu::Title;

  int drawMenu(Game &game) {

    bool lastModifier = shiftModifier;

    shiftModifier = IsKeyDown(KEY_LEFT_SHIFT);

    if (shiftModifier && !lastModifier) {
      for (auto &btn: levelButtons) {
        btn->text = std::to_string(std::stoi(btn->text) + 10);
      }
    } else if (!shiftModifier && lastModifier) {
      for (auto &btn: levelButtons) {
        btn->text = std::to_string(std::stoi(btn->text) - 10);
      }
    }
    


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

    // these have to be backwards.
    // todo: setup this animtaion
    auto paths = std::vector<std::string>{};
    for (const auto &path : paths) {
      titleAnimationFrames.push_back(LoadTexture(path.c_str()));
    }
  }

  void addTitleImageAnimation(rayui::Grid &grid) {
    // auto anim = grid.emplace_element<AnimatedImage>(Position{0,0},
    // mainMenuGrid.subdivisions, titleAnimationFrames); anim->framerateScale
    // = 2.0f;

    auto image = grid.emplace_element<rayui::Image>(
        Position{0, 0}, grid.subdivisions, titleImage);
    image->fillType = rayui::FillType::FillVertical;
    image->hAlignment = HAlignment::Center;
  }

  void setupSettingsMenu(Game &game) {

    addTitleImageAnimation(settingsGrid);

    settingsGrid.emplace_element<Rect>(Position{6, 13}, Size{11, 8},
                                       Style{GetColor(0x1b1b1bcc), WHITE},
                                       LayoutKind::None);

    *game.volumeLabel = "Volume: 100";
    auto volumeSlider = settingsGrid.emplace_element<Slider>(
        Position{7, 15}, Size{3, 2}, game.volumeLabel, 0, 100, 100,
        [&](float volume) {
          *game.volumeLabel = "Volume: " + std::to_string((int)volume);
          SetMasterVolume(volume / 100.0f);
        });

    volumeSlider->style.background = MAGENTA;
    volumeSlider->style.foreground = WHITE;
    volumeSlider->fontSize = 24;

    auto bagelButton = settingsGrid.emplace_element<Button>(
        Position{8, 16}, Size{7, 2}, "Toggle Bagel Mode", []() {}, buttonStyle);
    bagelButton->style.background = GREEN;
    bagelButton->onClicked = [bagelButton, &game]() {
      game.bagelMode = !game.bagelMode;
      bagelButton->style.background = game.bagelMode ? GREEN : RED;
    };

    auto pos = Position{9, 18};
    auto btn = settingsGrid.emplace_element<Button>(
        pos, Size{5, 2}, "Back", [this]() { this->menu = Menu::Title; },
        buttonStyle);
  }
  void setupTitleMenu() {
    addTitleImageAnimation(titleGrid);

    auto pos = Position{9, 18};
    auto btn = titleGrid.emplace_element<Button>(
        pos, Size{5, 2}, "Play", [this]() { this->menu = Menu::Main; },
        buttonStyle);

    btn->margin = {3, 3, 3, 3};

    pos.y += 2;
    auto btn1 = titleGrid.emplace_element<Button>(
        pos, Size{5, 2}, "Settings", [this]() { this->menu = Menu::Settings; },
        buttonStyle);

    btn1->margin = {3, 3, 3, 3};
  }
  void setupMainMenu(Game &game) {
    auto pos = Position{1, 21};
    auto size = Size{2, 2};

    addTitleImageAnimation(mainMenuGrid);

    mainMenuGrid.emplace_element<Rect>(Position{0, 19}, Size{1, 4},
                                       Style{GetColor(0x1b1b1bcc), WHITE},
                                       LayoutKind::StretchHorizontal);

    auto label = mainMenuGrid.emplace_element<Label>(
        Position{3, 19}, Size{1, 1}, "Level:", WHITE);

    mainMenuGrid.emplace_element<Label>(Position{17, 19}, Size{1, 1},
                                        "Other Modes:", WHITE);

    auto j = 0;
    for (int i = 0; i <= 9; ++i) {
      auto btnPos = Position{24 / 3 - 2 + i * size.width, pos.y - 2};
      auto callback = std::function<void()>([&, i = int(i)]() {
        game.reset();
        const int level = shiftModifier ? i + 10 : i;
        game.startLevel = level;
        game.level = level;
        game.scene = Game::Scene::InGame;
        game.gravity = game.gravityLevels[level];
      });

      if (i > 4) {
        btnPos.y += 2;
        btnPos.x = 24 / 3 - 2 + j;
        j += size.width;
      }

      auto button = mainMenuGrid.emplace_element<Button>(
          btnPos, size, std::to_string(i), callback, buttonStyle);
      pos.x += size.width;
      button->margin = {3, 3, 3, 3};
      levelButtons.push_back(button);
    }

    auto backButton = mainMenuGrid.emplace_element<Button>(
        Position{0, 20}, Size{2, 2}, "Back", [&]() { menu = Menu::Title; },
        buttonStyle);

    auto fortyLineBtn = mainMenuGrid.emplace_element<Button>(
        Position{20, 20}, Size{2, 2}, "40 lines",
        [&]() {
          game.mode = Game::Mode::FortyLines;
          game.reset();
          game.mode = Game::Mode::FortyLines;
          game.level = 5;
          game.startLevel = 5;
          game.scene = Game::Scene::InGame;
          game.gravity = game.gravityLevels[game.level];
        },
        buttonStyle);
    fortyLineBtn->fontSize = 18;
  }

  void setupGameOver(Game &game) {

    gameOverGrid.emplace_element<Rect>(Position{3, 3}, Size{18, 18},
                                       Style{GetColor(0x2b2b2bcc), WHITE});

    gameOverGrid.emplace_element<Label>(Position(8, 5), Size(2, 2),
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
        [&]() {
          game.scene = Game::Scene::MainMenu;
          menu = Menu::Main;
        },
        Style{BLACK, WHITE, BLACK, 3});

    gameOverGrid.emplace_element<Button>(
        Position{13, 17}, Size{5, 2}, "Retry",
        [&]() {
          game.reset();
          game.scene = Game::Scene::InGame;
        },
        Style{BLACK, WHITE, BLACK, 3});
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

Color RandomColor() {
  uint8_t r = GetRandomValue(0, 255);
  uint8_t g = GetRandomValue(0, 255);
  uint8_t b = GetRandomValue(0, 255);
  uint8_t a = GetRandomValue(0, 255);
  return Color{r, g, b, a};
}

struct FallingRect {
  Rectangle rect;
  Color color;
  float speed;
};


void pauseMenu() {
  static auto color = WHITE;
        static int ctr = 0;
        static std::vector<FallingRect> fallingRects;
        static bool initialized = false;
        
        if (!initialized) {
          for (int i = 0; i < 100; i++) {
            FallingRect newRect;
            newRect.rect.x = GetRandomValue(0, GetScreenWidth() - 20);
            newRect.rect.y = GetRandomValue(-200, -50);
            newRect.rect.width = GetRandomValue(20, 100);
            newRect.rect.height = GetRandomValue(20, 100);
            newRect.color = RandomColor();
            newRect.speed = GetRandomValue(2, 5);
            fallingRects.push_back(newRect);
          }
          initialized = true;
        }

        

        // Update and draw falling rectangles
        for (auto &rect : fallingRects) {
          // Update position
          rect.rect.y += rect.speed;

          // Draw rectangle
          DrawRectangleRec(rect.rect, rect.color);

          // Reset if it goes off screen
          if (rect.rect.y > GetScreenHeight()) {
            rect.rect.x = GetRandomValue(0, GetScreenWidth() - 20);
            rect.rect.y =
                GetRandomValue(-200, -50); // Reset to start above the screen
            rect.rect.width = GetRandomValue(20, 100);
            rect.rect.height = GetRandomValue(20, 100);
            rect.color = RandomColor();
            rect.speed = GetRandomValue(2, 5);
          }
        }
        auto size = MeasureText("Paused", 48);
        auto screenH = GetScreenHeight() / 2 - (size / 2),
             screenW = GetScreenWidth() / 2 - (size / 2);
        ClearBackground(BLACK);
        DrawText("Paused", screenW, screenH, 48, color);
        ctr++;
        if (ctr > 60) {
          color = RandomColor();
          color.a = 255;
          ctr = 0;
        }
}
int main(int argc, char *argv[]) {
  srand(time(0));
  InitWindow(800, 600, "boom taetris");
  InitAudioDevice();
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  SetTargetFPS(60);

  SetExitKey(KEY_END);

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
      ui.menu = UI::Menu::GameOver;
      ui.drawMenu(game);
      break;
    }
    case Game::Scene::InGame: {
      if (IsKeyPressed(KEY_ESCAPE)) {
        game.paused = !game.paused;
      }
      if (game.paused) {
        pauseMenu();
      } else {
        game.processGameLogic();
        game.drawGame();
      }
      break;
    }
    }
    EndDrawing();
  }

  game.scoreFile.write();
  CloseAudioDevice();
  return 0;
}