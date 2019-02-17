/** @file   sample.c
 *  @brief  sample use of tasks.
 *
 *  @author t-kenji <protect.2501@gmail.com>
 *  @date   2019-01-08 create new.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#include "tasks.h"

#define UNUSED_VARIABLE(x) (void)(x)

/**
 *  Startup routine.
 *
 *  @param  [in]    argc    Command-line arguments count.
 *  @param  [in]    argv    Command-line arguments vector.
 *  @return On success, zero is returned.
 *          On error, 1 is returned.
 */
int main(int argc, char *argv[])
{
    UNUSED_VARIABLE(argc);
    UNUSED_VARIABLE(argv);

    printf("Hello,World\n");

    return 0;
}
