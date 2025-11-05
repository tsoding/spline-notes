#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#define BUILD_DIR "build/"

Procs procs = {0};
Cmd cmd = {0};

void cc(void)
{
    cmd_append(&cmd, "cc");
    cmd_append(&cmd, "-Wall");
    cmd_append(&cmd, "-Wextra");
    cmd_append(&cmd, "-ggdb");
}

void raylib_cflags(void)
{
    cmd_append(&cmd, "-Iraylib-5.5_linux_amd64/include/");
}

void raylib_libs(void)
{
    cmd_append(&cmd, "-Lraylib-5.5_linux_amd64/lib/");
    cmd_append(&cmd, "-l:libraylib.a");
    cmd_append(&cmd, "-lm");
}

void freetype2_cflags(void)
{
    cmd_append(&cmd, "-I/usr/include/freetype2");
}

void freetype2_libs(void)
{
    cmd_append(&cmd, "-lfreetype");
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (!mkdir_if_not_exists(BUILD_DIR)) return 1;

    cc();
    raylib_cflags();
    cmd_append(&cmd, "-o", BUILD_DIR"spline");
    cmd_append(&cmd, "spline.c");
    raylib_libs();
    if (!cmd_run(&cmd, .async = &procs)) return 1;

    cc();
    raylib_cflags();
    freetype2_cflags();
    cmd_append(&cmd, "-o", BUILD_DIR"font");
    cmd_append(&cmd, "font.c");
    raylib_libs();
    freetype2_libs();
    if (!cmd_run(&cmd, .async = &procs)) return 1;

    if (!procs_flush(&procs)) return 1;

    return 0;
}
