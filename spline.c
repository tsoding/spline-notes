#include <stdio.h>
#include <stdbool.h>
#include <raylib.h>
#include <raymath.h>
#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#include "raster.c"

int main()
{
    InitWindow(window_width, window_height, "Splines");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GetColor(0x181818FF));
        if (IsKeyPressed(KEY_F2)) {
            spline.count = 0;
            for (size_t y = 0; y < grid_height; ++y) {
                for (size_t x = 0; x < grid_width; ++x) {
                    grid[y][x] = false;
                }
            }
        }
        edit_control_points();
        display_grid();
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
