#pragma once
#include <memory>
#include <raylib.h>
#include <vector>

namespace rayui {
struct Position {
  int x, y;
};

struct Size {
  int width, height;
};

struct Style {
  Style(Color bg, Color fg): background(bg), foreground(fg) {}
  Style(Color bg, Color fg, Color border, int borderSize): background(bg), foreground(fg), borderColor(border), borderSize(borderSize) {}
  virtual ~Style() {}
  Color background = BLACK;
  Color foreground = WHITE;
  Color borderColor = {0,0,0,0};
  int borderSize = 0;
};


// What other kinds of layout-kind might we want?
enum struct LayoutKind {
  // use the exact x,y & size coordinates provided by the element.
  None,
  // use the x & y coordinates but stretch to the full width of the container.
  StretchHorizontal,
  // use the x & y coordinates but stretch to the full height of the container.
  StretchVertical,
};

struct PixelPosition {
  float x, y;
};
struct PixelSize {
  float width, height;
};

// this object is passed to each element to tell it
// exactly where and how it should draw.
struct LayoutState {
  PixelPosition position;
  PixelSize size;
};

struct Margin {
  int top, left, bottom, right;
};

struct Element {
  Element(Position position, Size size,
          LayoutKind layoutKind = LayoutKind::None, Style style = {BLACK, WHITE})
      : style(style), position(position), size(size), layoutKind(layoutKind) {}
  Element() {}
  virtual ~Element() {}
  Style style = {BLACK, WHITE};
  LayoutKind layoutKind = LayoutKind::None;
  Position position = {0,0};
  Size size = {1,1};
  Margin margin = {0,0,0,0};
  virtual void draw(LayoutState &state) = 0;
};

// a container for a group of elements: a grid.
struct Grid : Element {
  template <typename T, typename... Args> 
  void emplace_element(Args &&...args) {
    auto element = std::make_unique<T>(args...);
    elements.push_back(std::move(element));
  }
  Grid(Position pos, Size size) : Element(pos, size) {}
  Grid() : Element() {}
  
  std::vector<std::unique_ptr<Element>> elements;

  void draw(LayoutState &state) override;

  Size subdivisions = {1, 1};
};

struct Rect : Element {
  Rect(Position position, Size size, Style style = {BLACK, WHITE},
       LayoutKind layoutKind = LayoutKind::None)
      : Element(position, size, layoutKind, style) {}

  void draw(LayoutState &state) override;
};

struct Label: Element {
  size_t fontSize = 12;
  char *text;
  Label(Position pos, Size size) : Element(pos, size) {}
  void draw(LayoutState &state) override {
    DrawRectangle(state.position.x, state.position.y, state.size.width, state.size.height, style.background);
    DrawText(text, state.position.x, state.position.y, fontSize, style.foreground);
  }
};

struct Button : Element {
  char *text = (char*)"";
  size_t fontSize;
  void (*onClicked)() = nullptr;
  
  Button(Position position, Size size, Style style = {BLACK, WHITE, WHITE, 0},
         LayoutKind layoutKind = LayoutKind::None)
      : Element(position, size, layoutKind, style) {}
  
  void draw(LayoutState &state) override {
    bool isMouseOver = CheckCollisionPointRec(GetMousePosition(), { state.position.x, state.position.y, state.size.width, state.size.height });
    if (isMouseOver && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      if (onClicked) {
        onClicked();
      }
    }
    
    if (isMouseOver) {
      DrawRectangle(state.position.x, state.position.y, state.size.width, state.size.height, style.background);
      DrawText(text, state.position.x, state.position.y, fontSize, style.foreground);
    } else {
      DrawRectangleLines(state.position.x, state.position.y, state.size.width, state.size.height, style.borderColor);
      DrawText(text, state.position.x, state.position.y, fontSize, style.foreground);
    }
  }
};


} // end namespace rayui
