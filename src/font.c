// TTF font renderer using the buggy rasterizer in raster.c
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
    Point* items;
    size_t count;
    size_t capacity;
} Points;

int append_to_spline(Point* const p1, Point* const p2, Vector2* p, Spline* spline) {
    Segment seg;
    int advance;
    if(p1->on) {
        seg = (Segment){
            .kind = SEGMENT_LINE,
            .p1 = *p,
            .p2 = p1->position,
        };
        *p = seg.p2;
        advance = 1;
    } else if(p2->on) {
        seg = (Segment){
            .kind = SEGMENT_QUAD,
            .p1 = *p,
            .p2 = p1->position,
            .p3 = p2->position,
        };
        *p = seg.p3;
        advance = 2;
    } else {
        seg = (Segment){
            .kind = SEGMENT_QUAD,
            .p1 = *p,
            .p2 = p1->position,
            .p3 = Vector2Lerp(p1->position, p2->position, 0.5),
        };
        *p = seg.p3;
        advance = 1;
    }
    da_append(spline, seg);
    return advance;
}

void _Spline(Spline* ptr) { da_free(*ptr); }
void _Points(Points* ptr) { da_free(*ptr); }

#define printf(...) // NOTE supress output

void rasterize(FT_Face face, int ch, float scale) {
    FT_Error error = FT_Load_Char(face, ch, FT_LOAD_DEFAULT);
    // error = FT_Load_Glyph(face, 'Q', FT_LOAD_DEFAULT);
    if(error) {
        fprintf(stderr, "ERROR: could not load glyph\n");
        return /*1*/;
    }

    if(!face->glyph->outline.n_points)
        return clear_grid(); // NOTE is empty, do nothing

    long int min_x = LONG_MAX, max_x = LONG_MIN;
    long int min_y = LONG_MAX, max_y = LONG_MIN;
    for(int i = 0; i < face->glyph->outline.n_points; ++i) {
        FT_Vector p = face->glyph->outline.points[i];
        unsigned char t = face->glyph->outline.tags[i];
        if(min_x > p.x) min_x = p.x;
        if(max_x < p.x) max_x = p.x;
        if(min_y > p.y) min_y = p.y;
        if(max_y < p.y) max_y = p.y;
        printf("%ld %ld %d\n", p.x, p.y, t);
    }

    printf("x = %ld .. %ld\n", min_x, max_x);
    printf("y = %ld .. %ld\n", min_y, max_y);

    printf("------------------------------\n");
    printf("n_contours = %d\n", face->glyph->outline.n_contours);
    printf("------------------------------\n");

    [[gnu::cleanup(_Points)]] Points points = {0};
    for(int pindex = 0; pindex <= face->glyph->outline.n_points; pindex++) {
        FT_Vector p = face->glyph->outline.points[pindex];
        unsigned char t = FT_CURVE_TAG(face->glyph->outline.tags[pindex]);
        assert(t != FT_CURVE_TAG_CUBIC);
        float x = (p.x - min_x) * scale + 100;
        float y = (max_y - p.y) * scale + 100;
        Point point = {
            .position = {x, y},
            .on = t == FT_CURVE_TAG_ON,
        };
        da_append(&points, point);
    }

    size_t contour_start = 0;
    [[gnu::cleanup(_Spline)]] Spline spline = {0};

    for(int i = 0; i < face->glyph->outline.n_contours; ++i) {
        size_t contour_size = face->glyph->outline.contours[i] - contour_start + 1;
        assert(contour_size > 2);

        Point* const contour_front = points.items + contour_start;
        Point* const contour_back = contour_front + contour_size - 1;
        Point* it = contour_front;
        Vector2 p = {0};
        if(contour_back->on) {
            p = contour_back->position;
        } else if(contour_front->on) {
            p = it++->position;
        } else {
            p = Vector2Lerp(contour_front->position, contour_back->position, 0.5);
        }
        while(it < contour_back)
            it += append_to_spline(it, it + 1, &p, &spline);
        append_to_spline(contour_back, contour_front, &p, &spline);

        contour_start = face->glyph->outline.contours[i] + 1;
    }
    printf("spline.count = %zu\n", spline.count);
    render_spline_into_grid(&spline);
}

int main() {
    FT_Library library = {0};

    FT_Error error = FT_Init_FreeType(&library);
    if(error) {
        fprintf(stderr, "ERROR: Could not initialize FreeType2 library\n");
        return 1;
    }

    const char* const font_file_path = "./assets/Vollkorn-Regular.ttf";

    FT_Face face;
    error = FT_New_Face(library, font_file_path, 0, &face);
    if(error == FT_Err_Unknown_File_Format) {
        fprintf(stderr, "ERROR: `%s` has an unknown format\n", font_file_path);
        return 1;
    } else if(error) {
        fprintf(stderr, "ERROR: Could not load file `%s`\n", font_file_path);
        return 1;
    }

    int factor = 100;
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(16 * factor, 9 * factor, "main");
    SetTargetFPS(60);
    int ctr = 0;
    int ch = L' ';
    // int ch = '#';
    // int ch = 'x';
    // int ch = 'Q';
    // int ch = '?';
    // int ch = 'O';
    // int ch = L'Ы';
    // int ch = L'繁';//汉仪菱心体
    // rasterize(face,ch, 0.5);
    while(!WindowShouldClose()) {
        ClearBackground(BLACK);
        BeginDrawing();
        if(!(++ctr % 5)) rasterize(face, ch++, 0.5);
        display_grid();
        EndDrawing();
    }
    CloseWindow();

    FT_Done_FreeType(library);

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
