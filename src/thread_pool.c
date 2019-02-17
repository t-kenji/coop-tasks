/** @file       thread_pool.c
 *  @brief      Thread pool implementation.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2019-01-08 create new.
 *  @copyright  Copyright (c) 2018-2019 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#include "utils.h"
#include "collections.h"
#include "future.h"
#include "threads.h"
#include "debug.h"
#include "thread_pool.h"

#define MAX_JOBS (32)

enum worker_state {
    INIT,
    IDLE,
    WORKING,
    FAIL,
};

#define JOB_MAKER(f, a)    \
    (job_t){               \
        .id = 0,           \
        .start_time = 0,   \
        .end_time = 0,     \
        .func = (f),       \
        .arg = (a),        \
        .name = {0},       \
        .waitable = false, \
    }

struct worker {
    thrd_t thr;
    pid_t wid;
    enum worker_state status;
    struct worker *colleagues;
    que_t *global_jobs;
    pthread_mutex_t *mtx;
    pthread_cond_t *cnd;
    _Atomic(size_t) *num_active;
    _Atomic(size_t) *num_local_jobs;
    que_t local_jobs;
    job_t job;
};

#define WORKER_MAKER(i, o)            \
    (struct worker){                  \
        .wid = (i),                   \
        .status = INIT,               \
        .colleagues = (o)->workers,   \
        .global_jobs = &(o)->jobs,    \
        .mtx = &(o)->seek_mtx,        \
        .cnd = &(o)->seek_cnd,        \
        .num_active = &(o)->num_active, \
        .num_local_jobs = &(o)->num_local_jobs, \
        .job = JOB_MAKER(NULL, NULL), \
    }

struct thread_pool {
    size_t num_workers;
    atomic_flag initialized;
    promise_t prms;
    future_t *ftr;
    que_t jobs;
    pthread_mutex_t seek_mtx;
    pthread_cond_t seek_cnd;
    _Atomic(size_t) num_active;
    _Atomic(size_t) num_local_jobs;
    struct worker workers[];
};

#define THREAD_POOL_MAKER(n)                   \
    (struct thread_pool){                      \
        .num_workers = (n),                    \
        .initialized = ATOMIC_FLAG_INIT,       \
        .prms = PROMISE_INITIALIZER,           \
        .ftr = NULL,                           \
        .seek_mtx = PTHREAD_MUTEX_INITIALIZER, \
        .seek_cnd = PTHREAD_COND_INITIALIZER,  \
        .num_active = ATOMIC_VAR_INIT(0),      \
        .num_local_jobs = ATOMIC_VAR_INIT(0),  \
    }

static _Thread_local struct worker *ctx = NULL;

int thrdpool_job_init(job_t *job, int (*func)(void *), void *arg)
{
    if ((job == NULL) || (func == NULL)) {
        errno = EINVAL;
        return -1;
    }

    *job = JOB_MAKER(func, arg);

    return 0;
}

int thrdpool_job_set_name(job_t *job, const char *name)
{
    if ((job == NULL) || (job->func == NULL) || (name == NULL)) {
        errno = EINVAL;
        return -1;
    }

    strncpy(job->name, name, sizeof(job->name) - 1);

    return 0;
}

int thrdpool_job_set_waitable(job_t *job, bool waitable)
{
    if ((job == NULL) || (job->func == NULL)) {
        errno = EINVAL;
        return -1;
    }

    job->waitable = waitable;

    return 0;
}

STATIC int work_steal(struct worker *self, job_t *job)
{
    for (int i = 0; self->colleagues[i].wid != -1; ++i) {
        struct worker *victim = &self->colleagues[i];
        if (!thrd_equal(victim->thr, self->thr)) {
            if (queue_dequeue(&victim->local_jobs, job) == 0) {
                return 0;
            }
        }
    }

    errno = ENOENT;
    return -1;
}

STATIC int job_seeking(struct worker *self, job_t *job)
{
    if (atomic_load(self->num_local_jobs) > 0) {
        if ((queue_dequeue(&self->local_jobs, job) == 0) || (work_steal(self, job) == 0)) {
            atomic_fetch_sub(self->num_local_jobs, 1);
            return 0;
        }
    }
    if (queue_dequeue(self->global_jobs, job) == 0) {
        return 0;
    }

    return -1;
}

STATIC int worker(void *arg)
{
    SELFLIZE(struct worker *, arg);

    ctx = self;
    atomic_store(&self->status, IDLE);

    char name[32];
    snprintf(name, sizeof(name), "worker[%d]", self->wid);
    thrd_set_name(thrd_current(), name);

    while (pthread_testcancel(), true) {
        job_t job;

        lock (self->mtx) {
            while (job_seeking(self, &job) != 0) {
                pthread_cond_wait(self->cnd, self->mtx);
            }
        }

        atomic_store(&self->job, job);
        if (job.name[0] != '\0') {
            thrd_set_name(self->thr, job.name);
        }
        atomic_fetch_add(self->num_active, 1);
        job.func(job.arg);
        atomic_fetch_sub(self->num_active, 1);
        if (job.name[0] != '\0') {
            thrd_set_name(self->thr, name);
        }
    }

    return 0;
}

STATIC int worker_creator(void *arg)
{
    SELFLIZE(struct thread_pool *, arg);

    for (size_t i = 1; i < self->num_workers; ++i) {
        struct worker *w = &self->workers[i];
        pthread_testcancel();

        if (thrd_create(&w->thr, worker, w) != 0) {
            atomic_store(&w->status, FAIL);
        }
    }
    promise_set_value(&self->prms, 0);

    return worker(&self->workers[0]);
}

tpool_t thrdpool_create(size_t num_workers)
{
    if (num_workers == 0) {
        errno = EINVAL;
        return NULL;
    }

    size_t workers_size = sizeof(struct worker) * (num_workers + 1);
    struct thread_pool *self = malloc(sizeof(*self) + workers_size);
    if (self == NULL) {
        return NULL;
    }

    *self = THREAD_POOL_MAKER(num_workers);
    if (queue_create(&self->jobs, sizeof(job_t), MAX_JOBS) != 0) {
        free(self);
        return NULL;
    }
    self->ftr = promise_get_future(&self->prms);
    for (size_t i = 0; i < num_workers; ++i) {
        struct worker *w = &self->workers[i];
        *w = WORKER_MAKER(i + 1, self);
        if (queue_create(&w->local_jobs, sizeof(job_t), MAX_JOBS) != 0) {
            queue_destroy(&self->jobs);
            free(self);
            return NULL;
        }
    }
    self->workers[num_workers] = WORKER_MAKER(-1, self);

    int ret = thrd_create(&self->workers[0].thr, worker_creator, self);
    if (ret != 0) {
        thrdpool_destroy(self);
        return NULL;
    }

    return self;
}

void thrdpool_destroy(tpool_t tp)
{
    if (tp == NULL) {
        return;
    }

    SELFLIZE(struct thread_pool *, tp);

    future_get_value(self->ftr, NULL);
    for (int i = (int)self->num_workers - 1; i >= 0; --i) {
        struct worker *w = &self->workers[i];
        if (thrd_cancel(w->thr) == 0) {
            thrd_join(w->thr, NULL);
        }
    }
    for (int i = (int)self->num_workers - 1; i >= 0; --i) {
        struct worker *w = &self->workers[i];
        queue_destroy(&w->local_jobs);
    }
    queue_destroy(&self->jobs);
    free(self);
}

ssize_t thrdpool_num_workers(tpool_t tp)
{
    if (tp == NULL) {
        errno = EINVAL;
        return -1;
    }

    SELFLIZE(struct thread_pool *, tp);

    return self->num_workers;
}

int thrdpool_add(tpool_t tp, job_t *job)
{
    if ((tp == NULL) || (job == NULL)) {
        errno = EINVAL;
        return -1;
    }

    SELFLIZE(struct thread_pool *, tp);

    static _Atomic(juid_t) juid = ATOMIC_VAR_INIT(0);

    job->id = atomic_fetch_add(&juid, 1);

    int ret;
    lock (&self->seek_mtx) {
        if (ctx != NULL) {
            ret = queue_enqueue(&ctx->local_jobs, job);
            if (ret == 0) {
                atomic_fetch_add(&self->num_local_jobs, 1);
            }
        } else {
            ret = queue_enqueue(&self->jobs, job);
        }
        pthread_cond_broadcast(&self->seek_cnd);
    }

    return (ret == 0) ? 0 : -1;
}
