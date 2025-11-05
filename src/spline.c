// Spline editor using the buggy rasterizer in raster.c
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
    Control_Points control_points = {
        .dragging = -1,
    };
    Spline spline = {0};

    InitWindow(window_width, window_height, "Splines");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GetColor(0x181818FF));
        if (IsKeyPressed(KEY_F2)) {
            control_points.count = 0;
            for (size_t y = 0; y < grid_height; ++y) {
                for (size_t x = 0; x < grid_width; ++x) {
                    grid[y][x] = false;
                }
            }
        }
        edit_control_points(&control_points, &spline);
        display_grid();
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
