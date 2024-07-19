#include "rayui.hpp"
#include <algorithm>

using namespace rayui;

void Grid::draw(LayoutState &state) {
  // Calculate the size of each cell in the grid
  float cellWidth = state.size.width / subdivisions.width;
  float cellHeight = state.size.height / subdivisions.height;

  for (const auto &element : elements) {
    LayoutState elementState;
    // keep element in bounds of grid
    auto maxElementX = subdivisions.width - std::max(element->size.width, 1);
    auto maxElementY = subdivisions.height - std::max(element->size.height, 1);
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
      elementState.size.height = pixelSizeY; // Use the calculated pixel height
      break;
    case LayoutKind::StretchVertical:
      elementState.position.x = pixelPosX;
      elementState.position.y = state.position.y;
      elementState.size.width = pixelSizeX; // Use the calculated pixel width
      elementState.size.height = state.size.height; // Stretch vertically
      break;
    }

    element->draw(elementState);
  }
}

void rayui::Rect::draw(LayoutState &state) {
  DrawRectangle(state.position.x, state.position.y, state.size.width,
                state.size.height, style.background);
}