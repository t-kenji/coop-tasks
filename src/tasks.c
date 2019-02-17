/** @file       tasks.c
 *  @brief      C library for task system.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2019-02-03 create new.
 *  @copyright  Copyright (c) 2018-2019 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#include "tasks.h"

#define RUNNABLE_INITIALIZER(f, a) \
    (runnable_t){                  \
        .tuid = 0,                 \
        .func = (f),               \
        .arg = (a),                \
        .name = NULL,              \
    }

int tasks_runnable_init(runnable_t *rnbl, int (*func)(void *), void *arg)
{
    if ((rnbl == NULL) || (func == NULL)) {
        errno = EINVAL;
        return -1;
    }

    *rnbl = RUNNABLE_INITIALIZER(func, arg);

    return 0;
}

int tasks_runnable_set_name(runnable_t *rnbl, const char *name)
{
    if ((rnbl == NULL) || (name == NULL)) {
        errno = EINVAL; 
        return -1;
    }

    rnbl->name = name;

    return 0;
}
