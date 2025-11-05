// The grid spline rasterizer which is reused by several applications in this repo.
#define width_factor 4
#define height_factor 3
#define windows_factor 200
#define window_width (width_factor*windows_factor)
#define window_height (height_factor*windows_factor)
#define grid_factor 25
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

typedef enum {
    SEGMENT_LINE,
    SEGMENT_QUAD,
} Segment_Kind;

typedef struct {
    Segment_Kind kind;
    Vector2 p1, p2, p3;
} Segment;

typedef struct {
    Segment *items;
    size_t count;
    size_t capacity;
} Spline;

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

void solve_y_quad(float y, Vector2 p1, Vector2 p2, Vector2 p3, Solutions *solutions)
{
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

void solve_y_line(float y, Vector2 p1, Vector2 p2, Solutions *solutions)
{
    float dy = p2.y - p1.y;
    if (fabsf(dy) > 1e-6) {
        float t = (y - p1.y)/dy;
        if (0 <= t && t <= 1) {
            float dx = p2.x - p1.x;
            float tx = dx*t + p1.x;
            float d = dy;
            Solution s = {tx, d};
            da_append(solutions, s);
        }
    }
}

void solve_row(const Spline *spline, size_t row, Solutions *solutions)
{
    solutions->count = 0;
    float y = (row + 0.5)*cell_height;

    for (size_t i = 0; i < spline->count; ++i) {
        Segment seg = spline->items[i];
        switch (seg.kind) {
        case SEGMENT_LINE:
            solve_y_line(y, seg.p1, seg.p2, solutions);
            break;
        case SEGMENT_QUAD:
            solve_y_quad(y, seg.p1, seg.p2, seg.p3, solutions);
            break;
        default:
            UNREACHABLE("Segment_Kind");
        }
    }


    qsort(solutions->items, solutions->count, sizeof(*solutions->items), compare_solutions_by_tx);
}

void clear_grid() {
    for(size_t row = 0; row < grid_height; ++row) {
        for(size_t col = 0; col < grid_width; ++col) {
            grid[row][col] = false;
        }
    }
}

void render_spline_into_grid(const Spline *spline)
{
    static Solutions solutions = {0};
    clear_grid();
    for (size_t row = 0; row < grid_height; ++row) {
        int winding = 0;
        solve_row(spline, row, &solutions);
        for (size_t i = 0; i < solutions.count; ++i) {
            Solution s = solutions.items[i];
            if (winding > 0) {
                if (i > 0) {
                    Solution p = solutions.items[i-1];
                    int col1 = p.tx/cell_width;
                    if (col1 < 0) col1 = 0;
                    if (col1 >= grid_width) col1 = grid_width - 1;
                    int col2 = s.tx/cell_width;
                    if (col2 < 0) col2 = 0;
                    if (col2 >= grid_width) col2 = grid_width - 1;
                    for (int col = col1; col <= col2; ++col) {
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

typedef struct {
    Vector2 *items;
    size_t count;
    size_t capacity;
    int dragging;
} Control_Points;


void control_points_to_spline(const Control_Points *control_points, Spline *spline)
{
    spline->count = 0;

    if (control_points->count <= 2) return;

    for (size_t i = 0; i + 2 <= control_points->count; i += 2) {
        Vector2 p1 = control_points->items[i];
        Vector2 p2 = control_points->items[i+1];
        Vector2 p3 = control_points->items[(i+2)%control_points->count];
        Segment seg = {
            .kind = SEGMENT_QUAD,
            .p1 = p1,
            .p2 = p2,
            .p3 = p3,
        };
        da_append(spline, seg);
    }

    if (control_points->count%2 == 1) {
        Vector2 p1 = control_points->items[control_points->count-1];
        Vector2 p2 = control_points->items[0];
        Segment seg = {
            .kind = SEGMENT_LINE,
            .p1 = p1,
            .p2 = p2,
        };
        da_append(spline, seg);
    }
}

void edit_control_points(Control_Points *control_points, Spline *spline)
{
    Vector2 mouse = GetMousePosition();

    for (size_t i = 0; i < control_points->count; ++i) {
        Vector2 size = {20, 20};
        Vector2 position = control_points->items[i];
        position = Vector2Subtract(position, Vector2Scale(size, 0.5));

        bool hover = CheckCollisionPointRec(mouse, (Rectangle) {
            position.x, position.y, size.x, size.y
        });

        if (hover) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) control_points->dragging = i;
        } else {
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) control_points->dragging = -1;
        }
        DrawRectangleV(position, size, hover ? RED : BLUE);
    }

    if (control_points->dragging >= 0) {
        if (
            control_points->items[control_points->dragging].x != mouse.x ||
            control_points->items[control_points->dragging].y != mouse.y
        ) {
            control_points_to_spline(control_points, spline);
            render_spline_into_grid(spline);
        }
        control_points->items[control_points->dragging] = mouse;
    } else {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            da_append(control_points, mouse);
        }
    }
}
