#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

Cmd cmd = {0};

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    cmd_append(&cmd, "cc");
    cmd_append(&cmd, "-Wall");
    cmd_append(&cmd, "-Wextra");
    cmd_append(&cmd, "-Iraylib-5.5_linux_amd64/include/");
    cmd_append(&cmd, "-o", "spline");
    cmd_append(&cmd, "main.c");
    cmd_append(&cmd, "-lm");
    cmd_append(&cmd, "-Lraylib-5.5_linux_amd64/lib/");
    cmd_append(&cmd, "-l:libraylib.a");
    if (!cmd_run(&cmd)) return 1;
    return 0;
}
