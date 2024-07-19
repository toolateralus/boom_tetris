#include "rayui.hpp"
#include <raylib.h>

int main(int argc, char *argv[]) {
	
	rayui::Grid grid;
	grid.subdivisions = {16, 16};
	
	grid.emplace_element<rayui::Rect>(rayui::Position{0, 0}, rayui::Size{2, 2}, rayui::Style {GREEN, RED}, rayui::LayoutKind::None);
	grid.emplace_element<rayui::Rect>(rayui::Position{2, 0}, rayui::Size{2, 2}, rayui::Style {GREEN, RED}, rayui::LayoutKind::None);
	
	InitWindow(0, 0, "ui test");
	
	auto &element = grid.elements[0];
	
	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(GetColor(0x12121212));
		
		if (IsKeyPressed(KEY_S)) {
			element->position.y++;
		}
		if (IsKeyPressed(KEY_D)) {
			element->position.x++;
		}
		if (IsKeyPressed(KEY_A)) {
			element->position.x--;
		}
		if (IsKeyPressed(KEY_W)) {
			element->position.y--;
		}
		
		if (IsKeyPressed(KEY_R)) {
			element->layoutKind = (rayui::LayoutKind)(((int)element->layoutKind + 1) % ((int)rayui::LayoutKind::StretchVertical + 1));
		}
		
		rayui::LayoutState state({0,0}, {(float)GetScreenWidth(), (float)GetScreenHeight()});
		grid.draw(state);
		EndDrawing();
	}
	
	return 0;
}