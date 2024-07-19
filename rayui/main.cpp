#include "rayui.hpp"
#include <raylib.h>

using namespace rayui;

int main(int argc, char *argv[]) {
	
	Grid grid;
	grid.subdivisions = {16, 16};
	
	grid.emplace_element<Rect>(Position{0, 0}, Size{2, 2}, Style {GREEN, RED}, LayoutKind::None);
	grid.elements.back()->margin = {10,10,10,10};
	grid.emplace_element<Rect>(Position{2, 0}, Size{2, 2}, Style {GREEN, RED}, LayoutKind::None);
	grid.elements.back()->margin = {10,10,10,10};
	
	
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
			element->layoutKind = (LayoutKind)(((int)element->layoutKind + 1) % ((int)LayoutKind::StretchVertical + 1));
		}
		
		LayoutState state({0,0}, {(float)GetScreenWidth(), (float)GetScreenHeight()});
		grid.draw(state);
		EndDrawing();
	}
	
	return 0;
}