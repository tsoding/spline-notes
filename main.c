#include <ft2build.h>
#include FT_FREETYPE_H
#include <raylib.h>
#include <raymath.h>
#define NOB_STRIP_PREFIX
#include "nob.h"

#include "raster.c"

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

    error = FT_Load_Char(face, 'x', FT_LOAD_DEFAULT);
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

    int factor = 100;
    int pindex = 0;
    assert(face->glyph->outline.n_contours == 1);
    Spline spline = {0};
    /*
    for (int i = 0; i < face->glyph->outline.n_contours; ++i) {
        for (; pindex <= face->glyph->outline.contours[i]; pindex++) {
            FT_Vector p = face->glyph->outline.points[pindex];
            // unsigned char t = face->glyph->outline.tags[pindex];
            float scale = 0.5;
            float x = (p.x - min_x)*scale + 100;
            float y = (max_y - p.y)*scale + 100;
            Vector2 cp = {x, y};
            da_append(&spline, cp);
            // DrawCircle(x, y, 8, colors[i%ARRAY_LEN(colors)]);
            // DrawCircle(x, y, 8, t ? GREEN : RED);
        }
    }
    */

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
