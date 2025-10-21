#include <stdio.h>
#include <stdbool.h>
#include <raylib.h>
#include <raymath.h>
#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#define width_factor 4
#define height_factor 3
#define windows_factor 200
#define window_width (width_factor*windows_factor)
#define window_height (height_factor*windows_factor)
#define grid_factor 20
#define grid_width (width_factor*grid_factor)
#define grid_height (height_factor*grid_factor)
#define cell_width (window_width/grid_width)
#define cell_height (window_height/grid_height)

static bool grid[grid_height][grid_width] = {0};

void display_grid(void)
{
    for (size_t y = 0; y < grid_height; ++y) {
        for (size_t x = 0; x < grid_width; ++x) {
            if (grid[y][x]) {
                Vector2 marker_position = {x*cell_width, y*cell_height};
                Vector2 cell_size       = {cell_width, cell_height};
                Vector2 marker_size     = Vector2Scale(cell_size, 0.4);
                marker_position         = Vector2Add(marker_position, Vector2Scale(cell_size, 0.5));
                marker_position         = Vector2Subtract(marker_position, Vector2Scale(marker_size, 0.5));
                DrawRectangleV(marker_position, marker_size, RED);
            }
        }
    }
}

typedef struct {
    Vector2 *items;
    size_t count;
    size_t capacity;
} Spline;

static Spline spline = {0};
static int dragging = -1;

void render_spline_into_grid(void)
{
    for (size_t row = 0; row < grid_height; ++row) {
        int winding = 0;
        for (size_t col = 0; col < grid_width; ++col) {
            float x = (col + 0.5)*cell_width;
            float y = (row + 0.5)*cell_height;
            for (size_t i = 0; i + 2 <= spline.count; i += 2) {
                Vector2 p1 = spline.items[i];
                Vector2 p2 = spline.items[i+1];
                Vector2 p3 = spline.items[(i+2)%spline.count];

                float dx12 = p2.x - p1.x;
                float dx23 = p3.x - p2.x;
                float dy12 = p2.y - p1.y;
                float dy23 = p3.y - p2.y;

                float a = dy23 - dy12;
                float b = 2*dy12;
                float c = p1.y - y;
                float D = b*b - 4*a*c;

                if (D < 0.0) continue;

                float t[2];
                size_t tn = 0;
                t[tn++] = (-b + sqrtf(D))/(2*a);
                t[tn++] = (-b - sqrtf(D))/(2*a);
                for (size_t j = 0; j < tn; ++j) {
                    if (!(0 <= t[j] && t[j] <= 1)) continue;
                    float tx = (dx23 - dx12)*t[j]*t[j] + 2*dx12*t[j] + p1.x;
                    if (col*cell_width <= tx && tx <= (col + 1)*cell_width) {
                        float d = (dy23 - dy12)*t[j] + dy12;
                        if (d < 0) {
                            winding += 1;
                        } else if (d > 0) {
                            winding -= 1;
                        }
                    }
                }

                // float x = (dx23 - dx12)*t*t + 2*dx12*t + p1.x;
                // float y = (dy23 - dy12)*t*t + 2*dy12*t + p1.y;
                // a*t*t + b*t + c - y = 0;
            }
            if (winding > 0) {
                grid[row][col] = true;
            } else {
                grid[row][col] = false;
            }
        }
    }
}

void edit_control_points(void)
{
    Vector2 mouse = GetMousePosition();

    for (size_t i = 0; i < spline.count; ++i) {
        Vector2 size = {20, 20};
        Vector2 position = spline.items[i];
        position = Vector2Subtract(position, Vector2Scale(size, 0.5));

        bool hover = CheckCollisionPointRec(mouse, (Rectangle) {position.x, position.y, size.x, size.y});

        if (hover) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) dragging = i;
        } else {
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) dragging = -1;
        }
        DrawRectangleV(position, size, hover ? RED : BLUE);
    }

    if (dragging >= 0) {
        if (spline.items[dragging].x != mouse.x || spline.items[dragging].y != mouse.y) {
            render_spline_into_grid();
        }
        spline.items[dragging] = mouse;
    } else {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            da_append(&spline, mouse);
        }
    }
}

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
