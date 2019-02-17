/** @file       thread_pool.h
 *  @brief      Thread pool implementation.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2019-01-08 create new.
 *  @copyright  Copyright (c) 2018-2019 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#ifndef __TASKS_EXPORT_THREAD_POOL_H__
#define __TASKS_EXPORT_THREAD_POOL_H__

#if defined(__cplusplus)
extern "C" {
#endif

#define JOB_NAME_MAX (32)

typedef uint32_t juid_t;

typedef struct job {
    /* private */
    juid_t id;
    int64_t start_time;
    int64_t end_time;

    /* public */
    int (*func)(void *);
    void *arg;
    char name[JOB_NAME_MAX];
    bool waitable;
} job_t;

typedef struct thread_pool *tpool_t;

int thrdpool_job_init(job_t *job, int (*func)(void *), void *arg);
int thrdpool_job_set_name(job_t *job, const char *name);
int thrdpool_job_set_waitable(job_t *job, bool waitable);

tpool_t thrdpool_create(size_t num_workers);
void thrdpool_destroy(tpool_t tp);
ssize_t thrdpool_num_workers(tpool_t tp);
int thrdpool_add(tpool_t tp, job_t *job);

#if defined(__cplusplus)
}
#endif

#endif /* __TASKS_EXPORT_THREAD_POOL_H__ */
