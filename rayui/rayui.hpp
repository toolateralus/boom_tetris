#pragma once
#include <cstdint>
#include <memory>
#include <raylib.h>
#include <vector>

namespace rayui {
struct Position {
  uint16_t x, y;
};

struct Size {
  uint16_t width, height;
};

struct Style {
  Style(Color bg, Color fg): background(bg), foreground(fg) {}
  virtual ~Style() {}
  Color background = BLACK;
  Color foreground = WHITE;
};

struct BorederedStyle : Style {
  Color borderColor;
  uint16_t borderSize;
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

struct Element {
  Element(Position position, Size size,
          LayoutKind layoutKind = LayoutKind::None, Style style = {BLACK, WHITE})
      : style(style), position(position), size(size), layoutKind(layoutKind) {}

  virtual ~Element() {}
  Style style = {BLACK, WHITE};
  LayoutKind layoutKind = LayoutKind::None;
  Position position;
  Size size;
  virtual void draw(LayoutState &state) = 0;
};

// a container for a group of elements: a grid.
struct Grid : Element {
  template <typename T, typename... Args> 
  void emplace_element(Args &&...args) {
    auto element = std::make_unique<T>(args...);
    elements.push_back(std::move(element));
  }
  
  // pixel position of the top-left of the grid.
  PixelPosition origin;
  // pixel size of the grid.
  PixelSize size;

  std::vector<std::unique_ptr<Element>> elements;

  void computePixelSpace();
  
  // draw from this element as root.
  void draw() {
    auto state = LayoutState { origin, size };
    this->draw(state);
  }
  
  void draw(LayoutState &state) override;

  // percentage based values used to compute pixel origin and size.
  const float origin_x, origin_y, size_x, size_y;

  const int subdivisions_x, subdivisions_y;

  // origin_x and origin_y are 0.0 to 1.0 values specifying at what percentage
  // of the screen will the grid begin.
  // size_x and size_y are 0.0 to 1.0 values specifying how much of the screen
  // this grid should take up.
  Grid(Position position, Size size, int subdivisions_x, int subdivisions_y, float origin_x,
            float origin_y, float size_x, float size_y);
};

struct Rect : Element {
  Rect(Position position, Size size, Style style = {BLACK, WHITE},
       LayoutKind layoutKind = LayoutKind::None)
      : Element(position, size, layoutKind, style) {}

  void draw(LayoutState &state) override;
};

struct Label: Element {
  Label(Position position, Size size, LayoutKind layoutKind = {}, Style style = {BLACK, WHITE}) : Element(position, size, layoutKind, style) {
    
  }
    
  
};

} // end namespace rayui
