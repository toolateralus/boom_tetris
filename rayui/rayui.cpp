#include "rayui.hpp"

using namespace rayui;

Grid::Grid(Position position, Size size, int subdivisions_x,
                     int subdivisions_y, float origin_x, float origin_y,
                     float size_x, float size_y)
    : Element(position, size), subdivisions_x(subdivisions_x),
      subdivisions_y(subdivisions_y), origin_x(origin_x), origin_y(origin_y),
      size_x(size_x), size_y(size_y) {
  computePixelSpace();
}

void Grid::draw(LayoutState &state) {
  computePixelSpace(); // Adjust container size based on screen size
  
  // Calculate the size of each cell in the grid
  float cellWidth = size.width / subdivisions_x;
  float cellHeight = size.height / subdivisions_y;

  for (const auto &element : elements) {
    // Convert grid position and size to pixel values
    float pixelPosX = origin.x + element->position.x * cellWidth;
    float pixelPosY = origin.y + element->position.y * cellHeight;
    float pixelSizeX = element->size.width * cellWidth;
    float pixelSizeY = element->size.height * cellHeight;

    switch (element->layoutKind) {
    case LayoutKind::None:
      state.position = {pixelPosX, pixelPosY};
      state.size = {pixelSizeX, pixelSizeY};
      break;
    case LayoutKind::StretchHorizontal:
      state.position.x = pixelPosX;
      state.position.y = pixelPosY;
      state.size.width = this->size.width - pixelPosX; // Stretch horizontally
      state.size.height = pixelSizeY; // Use the calculated pixel height
      break;
    case LayoutKind::StretchVertical:
      state.position.x = pixelPosX;
      state.position.y = pixelPosY;
      state.size.width = pixelSizeX; // Use the calculated pixel width
      state.size.height = this->size.height - pixelPosY; // Stretch vertically
      break;
    }

    element->draw(state);
  }
}
void rayui::Grid::computePixelSpace() {
  float x = origin_x * GetScreenWidth();
  float y = origin_y * GetScreenHeight();
  float sx = size_x * GetScreenWidth();
  float sy = size_y * GetScreenHeight();
  origin = {x, y};
  size = {sx, sy};
}

void rayui::Rect::draw(LayoutState &state) {
  DrawRectangle(state.position.x, state.position.y, state.size.width,
                state.size.height, style.background);
}