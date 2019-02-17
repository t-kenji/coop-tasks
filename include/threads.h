/** @file       threads.h
 *  @brief      C11 threads like thread component.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2019-02-09 create new.
 *  @copyright  Copyright (c) 2018-2019 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#ifndef __TASKS_EXPORT_THREADS_H__
#define __TASKS_EXPORT_THREADS_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <sched.h>
#include <pthread.h>

/**
 *  MAX_THREADS desc.
 */
#define MAX_THREADS (256)

/**
 *  thrd_t desc.
 */
typedef pthread_t thrd_t;

/**
 *  thrd_start_t desc.
 */
typedef int (*thrd_start_t)(void *);

/**
 *  thrd_create summary.
 */
int thrd_create(thrd_t *thr, thrd_start_t func, void *arg);

/**
 *  thrd_current summary.
 */
thrd_t thrd_current(void);

/**
 *  thrd_detach summary.
 */
int thrd_detach(thrd_t thr);

/**
 *  thrd_equal summary.
 */
int thrd_equal(thrd_t thr0, thrd_t thr1);

/**
 *  thrd_exit summary.
 */
void thrd_exit(int res);

/**
 *  thrd_join summary.
 */
int thrd_join(thrd_t thr, int *res);

/**
 *  thrd_sleep summary.
 */
void thrd_sleep(struct timespec *ts);

/**
 *  thrd_yield summary.
 */
void thrd_yield(void);

/**
 *  thrd_suspend summary.
 */
int thrd_suspend(thrd_t thr);

/**
 *  thrd_resume summary.
 */
int thrd_resume(thrd_t thr);

/**
 *  thrd_cancel summary.
 */
int thrd_cancel(thrd_t thr);

/**
 *  thrd_raise summary.
 */
int thrd_raise(int sig);

/**
 *  thrd_kill summary.
 */
int thrd_kill(thrd_t thr, int sig);

/**
 *  thrd_set_name summary.
 */
int thrd_set_name(thrd_t thr, const char *name);

/**
 *  thrd_get_name summary.
 */
int thrd_get_name(thrd_t thr, char *name, size_t len);

int thrd_set_prior(thrd_t thr, int prior);
int thrd_get_prior(thrd_t thr);
int thrd_set_affinity(thrd_t thr, const cpu_set_t *cpuset);
int thrd_get_affinity(thrd_t thr, cpu_set_t *cpuset);

#if defined(__cplusplus)
}
#endif

#endif /* __TASKS_EXPORT_THREADS_H__ */
