#pragma once
#include "raylib.h"

#include "rayui.hpp"
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <memory>
#include <stdio.h>
#include <unordered_map>
#include <vector>

#include "score.hpp"
#include <optional>

constexpr int boardWidth = 10;
constexpr int boardHeight = 20;

#define BG_COLOR GetColor(0x12121212)

using namespace rayui;

namespace boom_tetris {

// the direction of user input.
enum struct Direction { None, Left, Right, Down };
// the shape of a tetromino, a group of cells.
enum struct Shape { L, J, Z, S, I, T, O };
// the rotation of a tetromino
enum struct Orientation { Up, Right, Down, Left };
// an integer based vec2.
struct Vec2 {
  int x, y;

  Vec2 operator+(const Vec2 &other) const;

  Vec2 rotated(Orientation orientation) const;
};
// an image associated with a cell.
struct Block {
  Vec2 pos;
  size_t imageIdx;
};
// a way to key into the grid to update a tetromino.
using ShapeIndices = std::vector<Block>;
struct HorizontalInput {
  HorizontalInput(bool left, bool right) : left(left), right(right) {}
  bool left, right;
};
// a grid cell.
struct Cell {
  size_t imageIdx;
  bool empty = true;
};
struct Board {
  std::array<std::array<Cell, boardWidth>, boardHeight> rows = {};
  Cell &operator[](int x, int y);
  Cell &get_cell(int x, int y) { return (*this)[x, y]; }
  auto begin() noexcept { return rows.begin(); }
  auto end() noexcept { return rows.end(); }

  // We need more information that just whether it collided or not: we need to
  // know what side we hit so we can depenetrate in the opposite direction.
  bool collides(Vec2 pos) noexcept;
};

// a group of cells the user is currently in control of.
struct Tetromino {
  size_t softDropHeight = 0;
  Vec2 prev_position;
  Orientation prev_orientation;
  Vec2 position;
  Shape shape;
  Orientation orientation = Orientation::Up;

  // currently exist in, so that we can safely move into new cells.
  void spinRight();
  void spinLeft();
  void saveState();

  Tetromino() = delete;
  Tetromino(Shape &shape) {
    this->shape = shape;
    position = {5, 0};
  }
};

struct Game;

struct PieceViewer : Element {
  Game &game;
  virtual void draw(rayui::LayoutState &state) override;
  PieceViewer(Position position, Size size, Game &game)
      : Element(position, size), game(game) {}
};
struct BoardCell : Element {
  Game &game;
  Cell &cell;
  virtual void draw(rayui::LayoutState &state) override;
  BoardCell(Position position, Game &game, Cell &cell)
      : Element(position, {1, 1}), game(game), cell(cell) {}
};

struct Animation {
  Animation(Game *game) : game(game) {}
  Game *game;
  virtual ~Animation() {}
  virtual bool invoke() = 0;
};
struct CellDissolveAnimation : Animation {
  explicit CellDissolveAnimation(Game *game, std::vector<size_t> lines,
                                 size_t softDropHeight)
      : Animation(game), lines(std::move(lines)),
        softDropHeight(softDropHeight) {}
  size_t softDropHeight;
  std::vector<size_t> lines;
  int cellIdx = 0;
  bool invoke() override;
};
struct LockInAnimation : Animation {
  explicit LockInAnimation(Game *game, int pieceHeight)
      : Animation(game), pieceHeight(pieceHeight) {}
  size_t softDropHeight;
  int frameCount = 0;
  int pieceHeight = 0;
  bool invoke() override;
};

static int randInt(int maxInclusive = 1) {
  return rand() % (maxInclusive + 1);
}

struct Game {
  Sound shiftSound;
  Sound rotateSound;
  Sound lockInSound;
  Sound clearLineSound;
  
  bool paused = false;
  
  std::string *volumeLabel = new std::string();
  
  std::vector<Sound> dependencySounds = {};
  Sound johnnyDependencySound;
  std::vector<Sound> tetrisSounds = {};
  std::vector<Sound> bagelSounds = {};
  
  bool bagelMode = true;
  bool downLocked = false;
  
  ScoreFile scoreFile;
  size_t frameCount = 0;
  size_t dependencies;
  Grid gameGrid;
  
  std::deque<std::unique_ptr<Animation>> animation_queue = {};
  
  
  enum struct Mode {
    Normal,     // high score
    FortyLines, // timed 40 line clear.
  } mode = Mode::Normal;

  // the play grid.
  Board board;
  // the upcoming shape & color of the next tetromino.
  Shape nextShape;
  // the piece the player is in control of.
  std::optional<Tetromino> tetromino;
  // time since game start.
  std::chrono::milliseconds elapsed = std::chrono::milliseconds(0);
  // TODO: make this more like classic tetris.
  std::vector<float> gravityLevels;
  // different shape patterns.
  static std::unordered_map<Shape, std::vector<Block>> shapePatterns;
  // unit size of a cell on the grid, in pixels. based on resolution
  int blockSize = 32;
  // the block texture, used and tinted for every block.
  Texture2D blockTexture;
  // at which rate are we moving the tetromino down?
  float gravity = 0.0f;
  // extra gravity for when the player is holding down.
  float playerGravity = 0.0f;
  // current level
  size_t level = 0;
  // current score
  size_t score = 0;
  // the level this latest game started at.
  size_t startLevel = 0;

  size_t linesClearedThisLevel = 0;
  size_t totalLinesCleared = 0;

  // used for swapping between menus and the game.
  enum struct Scene { MainMenu, GameOver, InGame } scene;

  Game();
  ~Game();

  void reset();
  Grid createGrid();
  void drawGame();
  int findGamepad() const;

  void generateGravityLevels(int totalLevels);

  void setNextShape();
  void processGameLogic();
  
  std::vector<size_t> checkLines();
  void applyLineClearScoreAndLevel(size_t linesCleared);
  void applySoftDropScore(size_t softDropHeight);
  void saveTetromino();
  
  void playBoomDependency() const {
    static int i = 0;
    auto sound = dependencySounds[i++ % dependencySounds.size()];
    SetSoundVolume(sound, GetMasterVolume() + 0.25f);
    PlaySound(sound);
  }
  void playBoomTetris() const {
    static int i = 0;
    auto sound = tetrisSounds[i++ % tetrisSounds.size()];
    SetSoundVolume(sound, GetMasterVolume() + 0.25f);
    PlaySound(sound);
  }
  void playBoomBagel() const {
    static int i = 0;
    auto sound = bagelSounds[i++ % bagelSounds.size()];
    SetSoundVolume(sound, GetMasterVolume() + 0.25f);
    PlaySound(sound);
  }
  
  HorizontalInput delayedAutoShift();
  void cleanTetromino(std::optional<Tetromino> &tetromino);
  bool resolveCollision(std::optional<Tetromino> &tetromino);
  ShapeIndices
  getTransformedBlocks(std::optional<Tetromino> &tetromino) const;
  std::shared_ptr<rayui::Grid> createBoardGrid();

  int findLongBarDependencies() const;
};

} // namespace boom_tetris
