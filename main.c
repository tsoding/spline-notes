#include <ft2build.h>
#include FT_FREETYPE_H
#include <raylib.h>
#include <raymath.h>
#define NOB_STRIP_PREFIX
#include "nob.h"

#include "raster.c"

typedef struct {
    Vector2 position;
    bool on;
} Point;

typedef struct {
    Point *items;
    size_t count;
    size_t capacity;
} Points;

int main()
{
    FT_Library library = {0};

    FT_Error error = FT_Init_FreeType(&library);
    if (error) {
        fprintf(stderr, "ERROR: Could not initialize FreeType2 library\n");
        return 1;
    }

    const char *const font_file_path = "./Vollkorn-Regular.ttf";

    FT_Face face;
    error = FT_New_Face(library, font_file_path, 0, &face);
    if (error == FT_Err_Unknown_File_Format) {
        fprintf(stderr, "ERROR: `%s` has an unknown format\n", font_file_path);
        return 1;
    } else if (error) {
        fprintf(stderr, "ERROR: Could not load file `%s`\n", font_file_path);
        return 1;
    }

    // char c = '#';
    // char c = 'x';
    // char c = 'Q';
    // char c = '?';
    char c = 'O';
    error = FT_Load_Char(face, c, FT_LOAD_DEFAULT);
    // error = FT_Load_Glyph(face, 'Q', FT_LOAD_DEFAULT);
    if (error) {
        fprintf(stderr, "ERROR: could not load glyph\n");
        return 1;
    }

    long int min_x = LONG_MAX, max_x = LONG_MIN;
    long int min_y = LONG_MAX, max_y = LONG_MIN;
    for (int i = 0; i < face->glyph->outline.n_points; ++i) {
        FT_Vector p = face->glyph->outline.points[i];
        unsigned char t = face->glyph->outline.tags[i];
        if (min_x > p.x) min_x = p.x;
        if (max_x < p.x) max_x = p.x;
        if (min_y > p.y) min_y = p.y;
        if (max_y < p.y) max_y = p.y;
        printf("%ld %ld %d\n", p.x, p.y, t);
    }

    printf("x = %ld .. %ld\n", min_x, max_x);
    printf("y = %ld .. %ld\n", min_y, max_y);

    printf("------------------------------\n");
    printf("n_contours = %d\n", face->glyph->outline.n_contours);
    printf("------------------------------\n");

    int factor = 100;
    Points points = {0};
    for (int pindex = 0; pindex <= face->glyph->outline.n_points; pindex++) {
        FT_Vector p = face->glyph->outline.points[pindex];
        unsigned char t = FT_CURVE_TAG(face->glyph->outline.tags[pindex]);
        assert(t != FT_CURVE_TAG_CUBIC);
        float scale = 0.5;
        float x = (p.x - min_x)*scale + 100;
        float y = (max_y - p.y)*scale + 100;
        Point point = {
            .position = {x, y},
            .on = t == FT_CURVE_TAG_ON,
        };
        da_append(&points, point);
    }

    size_t contour_start = 0;
    Spline spline = {0};
    for (int i = 0; i < face->glyph->outline.n_contours; ++i) {
        Point *contour      = &points.items[contour_start];
        size_t contour_size = face->glyph->outline.contours[i] - contour_start + 1;
        assert(contour_size > 2);
        Vector2 p = {0};
        size_t j = 0;
        bool hack = false;
        if (contour[0].on) {
            p = contour[0].position;
            j = 1;
            hack = true;
        } else if (contour[contour_size - 1].on) {
            p = contour[contour_size - 1].position;
            j = 0;
        } else {
            p = Vector2Lerp(contour[0].position, contour[contour_size - 1].position, 0.5);
            j = 0;
        }
        printf("------------------------------\n");
        printf("j = %zu\n", j);
        printf("------------------------------\n");
        while ((hack && j <= contour_size) || j < contour_size) {
            if (contour[j%contour_size].on) {
                Segment seg = {
                    .kind = SEGMENT_LINE,
                    .p1 = p,
                    .p2 = contour[j%contour_size].position,
                };
                da_append(&spline, seg);
                p = contour[j%contour_size].position;
                j += 1;
            } else if (contour[(j+1)%contour_size].on) {
                Segment seg = {
                    .kind = SEGMENT_QUAD,
                    .p1 = p,
                    .p2 = contour[j%contour_size].position,
                    .p3 = contour[(j+1)%contour_size].position,
                };
                da_append(&spline, seg);
                p = contour[(j+1)%contour_size].position;
                j += 2;
            } else {
                Vector2 v = Vector2Lerp(
                    contour[j%contour_size].position,
                    contour[(j+1)%contour_size].position,
                    0.5
                );
                Segment seg = {
                    .kind = SEGMENT_QUAD,
                    .p1 = p,
                    .p2 = contour[j%contour_size].position,
                    .p3 = v,
                };
                da_append(&spline, seg);
                p = v;
                j += 1;
            }
        }
        contour_start = face->glyph->outline.contours[i] + 1;
    }

    printf("spline.count = %zu\n", spline.count);
    render_spline_into_grid(&spline);

    InitWindow(16*factor, 9*factor, "main");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        ClearBackground(BLACK);
        BeginDrawing();
        display_grid();
        EndDrawing();
    }
    CloseWindow();

    // Types we are interested in
    // - FT_Outline from ftimage.h
    // - FT_Vector
    //
    // - FT_OutlineGlyphRec
    // - FT_GlyphSlotRec
    // *   You can typecast an @FT_Glyph to @FT_OutlineGlyph if you have
    // *   `glyph->format == FT_GLYPH_FORMAT_OUTLINE`.  This lets you access the
    // *   outline's content easily.
    // - FT_Glyph


    printf("OK\n");
    return 0;
}
