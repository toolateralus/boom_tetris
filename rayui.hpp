#pragma once
#include <chrono>
#include <cstring>
#include <functional>
#include <memory>
#include <raylib.h>
#include <vector>

namespace rayui {
  
// the grid position of an element.
// if this exceeds the subdivisions of the grid, or goes below 0,
// it will be clamped to the furthest it can go.
struct Position {
  int x, y;
};

// the grid size of an element, {1,1} would take up exactly one cell.
struct Size {
  int width, height;
};

// Color and border information to describe the appearance of an element.
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


enum struct LayoutKind {
  // use the exact x,y & size coordinates provided by the element.
  None,
  // draw at the y coordinate but stretch to the full width of the container.
  StretchHorizontal,
  // draw at the x coordinate but stretch to the full height of the container.
  StretchVertical,
};

// the pixel position of an element; used internally for drawing.
struct PixelPosition {
  float x, y;
};

// the pixel size of an element; used internally for drawing.
struct PixelSize {
  float width, height;
};

// Margins to deviate from the exact grid cell coordinates.
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

// the base class for all UI elements.
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

// A basic colored rectangle.
struct Rect : Element {
  Rect(Position position, Size size, Style style = {},
       LayoutKind layoutKind = LayoutKind::None, Margin margin = {})
      : Element(position, size, layoutKind, style, margin) {}

  void draw(LayoutState &state) override {
    DrawRectangle(state.position.x, state.position.y, state.size.width,
                  state.size.height, style.background);
  }
};

// A simple text label.
struct Label : Element {
  std::string text;
  Label(Position pos, Size size, std::string text) : Element(pos, size), text(text) {}
  Label(Position pos, Size size, std::string text, Color foreground) : Element(pos, size), text(text) {
    style.foreground = foreground;
  }
  Label(Position pos, Size size) : Element(pos, size) {}
  void draw(LayoutState &state) override {
    DrawRectangle(state.position.x, state.position.y, state.size.width,
                  state.size.height, style.background);
    auto fontSize = state.size.height;
    DrawText(text.c_str(), state.position.x, state.position.y, fontSize,
             style.foreground);
  }
};

// A simple button with a mouse-over highlight, and an onClicked callback.
struct Button : Element {
  std::string text;
  size_t fontSize = 24;
  
  using ClickCallback = std::function<void()>;
  ClickCallback onClicked = nullptr;
  
  Button(Position position, Size size, std::string text = "",
         ClickCallback onClicked = nullptr,
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
    auto len = std::strlen(text.c_str());
    float size = ((float)fontSize / 2) * len;
    auto pos_x = state.position.x + (0.5 * state.size.width) - (size / 2);
    auto pos_y = state.position.y + (0.5 * state.size.height) - (fontSize / 2.0);
    
    DrawText(text.c_str(), pos_x, pos_y, fontSize, style.foreground);
  }
};

// An easy way to draw a texture within a UI.
struct Image : Element {
  // raylib image.
  ::Texture2D texture;
  bool loadedFromPath = false;
  
  // dictates the region of the image that will be drawn.
  // 0,0, imageWidth, imageHeight is default, and draws thw whole texture.
  Rectangle imageSourceRect = {};
  
  // the rotation of the texture. (in radians)
  float rotation = 0;
  
  // I don't really know what this field does but raylib takes it.
  Vector2 origin = {0,0};
  
  Image(Position position, Size size, std::string path,
         Style style = {BLACK, WHITE, WHITE, 0},
         LayoutKind layoutKind = LayoutKind::None)
      : Element(position, size, layoutKind, style) {
        texture = LoadTexture(path.c_str());      
        imageSourceRect = {0, 0, (float)texture.width, (float)texture.height};
        loadedFromPath = true;
      }
      
  Image(Position position, Size size, Texture2D &texture,
        Style style = {BLACK, WHITE, WHITE, 0},
        LayoutKind layoutKind = LayoutKind::None)
    : Element(position, size, layoutKind, style), texture(texture) {
      imageSourceRect = {0, 0, (float)texture.width, (float)texture.height};
    }
      
  void draw(LayoutState &state) override {
    Rectangle destRect = {state.position.x, state.position.y, state.position.x + state.size.width, state.position.y + state.size.height};    
    DrawTexturePro(texture, imageSourceRect, destRect, origin, rotation, style.background);
  }    
  ~Image() {
    if (loadedFromPath)
      UnloadTexture(texture);
  }
};

// attach a size_t * to this element so it displays a self-updating numerical value.
struct NumberText : Element {
  size_t *number;
  Color color;
  NumberText(Position position, Size size, size_t *number, Color color)
      : Element(position, size), number(number), color(color) {}
  virtual void draw(LayoutState &state) override {
    DrawText(std::to_string(*number).c_str(), state.position.x, state.position.y,
            state.size.height, color);
  }
};

// attach a std::chrono::milliseconds * to this element so it has a self updating time value.
struct TimeText : Element {
  std::chrono::milliseconds *time;
  Color color;
  TimeText(Position position, Size size, std::chrono::milliseconds *time, Color color)
      : Element(position, size), time(time), color(color) {}
  virtual void draw(LayoutState &state) override {
    auto totalMilliseconds = (*this->time).count() / 1000;
    int hours = totalMilliseconds / 3600;
    int minutes = (totalMilliseconds % 3600) / 60;
    int seconds = totalMilliseconds % 60;
    
    std::string timeStr;
    if (hours > 0) {
        timeStr = std::to_string(hours) + ":" + 
                  std::to_string(minutes).insert(0, 2 - std::to_string(minutes).length(), '0') + ":" + 
                  std::to_string(seconds).insert(0, 2 - std::to_string(seconds).length(), '0');
    } else {
        timeStr = std::to_string(minutes) + ":" + 
                  std::to_string(seconds).insert(0, 2 - std::to_string(seconds).length(), '0');
    }
    
    DrawText(timeStr.c_str(), state.position.x, state.position.y, state.size.height, color);
  }
};

} // end namespace rayui
