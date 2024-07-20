#pragma once
#include <cstring>
#include <functional>
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
  Style(Color bg, Color fg) : background(bg), foreground(fg) {}
  Style(Color bg, Color fg, Color border, int borderSize)
      : background(bg), foreground(fg), borderColor(border),
        borderSize(borderSize) {}
  Style() {}
  virtual ~Style() {}
  Color background = {0, 0, 0, 0};
  Color foreground = WHITE;
  Color borderColor = {0, 0, 0, 0};
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

struct Margin {
  int top, left, bottom, right;
};

// this object is passed to each element to tell it
// exactly where and how it should draw.
struct LayoutState {
  PixelPosition position;
  PixelSize size;
  void applyMargin(Margin margin) {
    size.width -= margin.left + margin.right;
    size.height -= margin.top + margin.bottom;
    position.x += margin.left;
    position.y += margin.top;
  }
};

struct Element {
  Element(Position position, Size size,
          LayoutKind layoutKind = LayoutKind::None, Style style = {},
          Margin margin = {})
      : style(style), position(position), size(size), layoutKind(layoutKind),
        margin(margin) {}
  Element() {}
  virtual ~Element() {}
  Style style;
  LayoutKind layoutKind = LayoutKind::None;
  Position position = {0, 0};
  Size size = {1, 1};
  Margin margin = {0, 0, 0, 0};
  virtual void draw(LayoutState &state) = 0;
};

// a container for a group of elements: a grid.
struct Grid : Element {
  template <typename T, typename... Args>
  std::shared_ptr<T> emplace_element(Args &&...args) {
    auto element = std::make_shared<T>(args...);
    elements.push_back(element);
    return element;
  }
  Grid(Position pos, Size size) : Element(pos, size) {}
  Grid(Size subdivisions = {1, 1}) : Element(), subdivisions(subdivisions) {}

  std::vector<std::shared_ptr<Element>> elements;

  void draw(LayoutState &state) override {
    // Calculate the size of each cell in the grid
    float cellWidth = state.size.width / subdivisions.width;
    float cellHeight = state.size.height / subdivisions.height;
    DrawRectangle(state.position.x, state.position.y, state.size.width,
                  state.size.height, style.background);

    for (const auto &element : elements) {
      LayoutState elementState;
      // keep element in bounds of grid
      auto maxElementX = subdivisions.width - std::max(element->size.width, 1);
      auto maxElementY =
          subdivisions.height - std::max(element->size.height, 1);
      auto elementX = std::clamp(element->position.x, 0, maxElementX);
      auto elementY = std::clamp(element->position.y, 0, maxElementY);
      // Convert grid position and size to pixel values
      float pixelPosX = state.position.x + elementX * cellWidth;
      float pixelPosY = state.position.y + elementY * cellHeight;
      float pixelSizeX = element->size.width * cellWidth;
      float pixelSizeY = element->size.height * cellHeight;

      switch (element->layoutKind) {
      case LayoutKind::None:
        elementState.position = {pixelPosX, pixelPosY};
        elementState.size = {pixelSizeX, pixelSizeY};
        break;
      case LayoutKind::StretchHorizontal:
        elementState.position.x = state.position.x;
        elementState.position.y = pixelPosY;
        elementState.size.width = state.size.width; // Stretch horizontally
        elementState.size.height =
            pixelSizeY; // Use the calculated pixel height
        break;
      case LayoutKind::StretchVertical:
        elementState.position.x = pixelPosX;
        elementState.position.y = state.position.y;
        elementState.size.width = pixelSizeX; // Use the calculated pixel width
        elementState.size.height = state.size.height; // Stretch vertically
        break;
      }

      elementState.applyMargin(element->margin);
      element->draw(elementState);
    }
  }

  Size subdivisions = {1, 1};
};

struct Rect : Element {
  Rect(Position position, Size size, Style style = {},
       LayoutKind layoutKind = LayoutKind::None, Margin margin = {})
      : Element(position, size, layoutKind, style, margin) {}

  void draw(LayoutState &state) override {
    DrawRectangle(state.position.x, state.position.y, state.size.width,
                  state.size.height, style.background);
  }
};

struct Label : Element {
  size_t fontSize = 12;
  char *text;
  Label(Position pos, Size size) : Element(pos, size) {}
  void draw(LayoutState &state) override {
    DrawRectangle(state.position.x, state.position.y, state.size.width,
                  state.size.height, style.background);
    DrawText(text, state.position.x, state.position.y, fontSize,
             style.foreground);
  }
};

struct DynamicLabel : Element {
  std::string text;
  DynamicLabel(Position pos, Size size) : Element(pos, size) {}
  void draw(LayoutState &state) override {
    DrawRectangle(state.position.x, state.position.y, state.size.width,
                  state.size.height, style.background);
    auto fontSize = state.size.height;
    DrawText(text.c_str(), state.position.x, state.position.y, fontSize,
             style.foreground);
  }
};

struct Button : Element {
  char *text = (char *)"";
  size_t fontSize = 24;
  using ClickedCallback = std::function<void()>;
  ClickedCallback onClicked = nullptr;

  Button(Position position, Size size, char *text = (char *)"",
         ClickedCallback onClicked = nullptr,
         Style style = {BLACK, WHITE, WHITE, 0},
         LayoutKind layoutKind = LayoutKind::None)
      : Element(position, size, layoutKind, style), text(text),
        onClicked(onClicked) {}

  void draw(LayoutState &state) override {
    bool isMouseOver = CheckCollisionPointRec(
        GetMousePosition(), {state.position.x, state.position.y,
                             state.size.width, state.size.height});
    if (isMouseOver && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      if (onClicked) {
        onClicked();
      }
    }

    if (isMouseOver) {
      DrawRectangle(state.position.x, state.position.y, state.size.width,
                    state.size.height, style.background);
    } else {
      DrawRectangleLines(state.position.x, state.position.y, state.size.width,
                         state.size.height, style.borderColor);
    }
    auto len = std::strlen(text);
    float size = ((float)fontSize / 2) * len;
    auto pos_x = state.position.x + (0.5 * state.size.width) - (size / 2);
    auto pos_y = state.position.y + (0.5 * state.size.height) - (fontSize / 2.0);
    
    DrawText(text, pos_x, pos_y, fontSize, style.foreground);
  }
};

} // end namespace rayui
