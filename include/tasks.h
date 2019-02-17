/** @file       tasks.h
 *  @brief      C library for task system.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2019-01-08 create new.
 *  @copyright  Copyright (c) 2018-2019 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#ifndef __TASKS_EXPORT_H__
#define __TASKS_EXPORT_H__

typedef uint32_t tuid_t;

typedef struct runnable {
    /* private */
    tuid_t tuid;

    /* public */
    int (*func)(void *);
    void *arg;
    const char *name; /**< @warning should be less than 16 chars. */
} runnable_t;

int tasks_runnable_init(runnable_t *rnbl, int (*func)(void *), void *arg);
int tasks_runnable_set_name(runnable_t *rnbl, const char *name);

#endif /* __TASKS_EXPORT_H__ */
