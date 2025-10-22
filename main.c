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

typedef struct {
    float tx;
    float d;
} Solution;

typedef struct {
    Solution *items;
    size_t count;
    size_t capacity;
} Solutions;

int compare_solutions_by_tx(const void *a, const void *b)
{
    const Solution *sa = a;
    const Solution *sb = b;
    if (sa->tx < sb->tx) return -1;
    if (sa->tx > sb->tx) return 1;
    return 0;
}

void solve_row(size_t row, Solutions *solutions)
{
    solutions->count = 0;
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

        float t[2];
        size_t tn = 0;

        if (fabsf(a) > 1e-6) {
            float D = b*b - 4*a*c;
            if (D >= 0.0) {
                t[tn++] = (-b + sqrtf(D))/(2*a);
                t[tn++] = (-b - sqrtf(D))/(2*a);
            }
        } else if (fabsf(b) > 1e-6) {
            t[tn++] = -c/b;
        }

        for (size_t j = 0; j < tn; ++j) {
            if (!(0 <= t[j] && t[j] <= 1)) continue;
            float tx = (dx23 - dx12)*t[j]*t[j] + 2*dx12*t[j] + p1.x;
            float d = (dy23 - dy12)*t[j] + dy12;
            Solution s = {tx, d};
            da_append(solutions, s);
        }
    }
    qsort(solutions->items, solutions->count, sizeof(*solutions->items), compare_solutions_by_tx);
}

void render_spline_into_grid(void)
{
    static Solutions solutions = {0};
    for (size_t row = 0; row < grid_height; ++row) {
        for (size_t col = 0; col < grid_width; ++col) {
            grid[row][col] = false;
        }
    }
    for (size_t row = 0; row < grid_height; ++row) {
        int winding = 0;
        solve_row(row, &solutions);
        for (size_t i = 0; i < solutions.count; ++i) {
            Solution s = solutions.items[i];
            if (winding > 0) {
                if (i > 0) {
                    Solution p = solutions.items[i-1];
                    size_t col1 = p.tx/cell_width;
                    size_t col2 = s.tx/cell_width;
                    for (size_t col = col1; col <= col2; ++col) {
                        grid[row][col] = true;
                    }
                }
            }
            if (s.d < 0) {
                winding += 1;
            } else if (s.d > 0) {
                winding -= 1;
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
