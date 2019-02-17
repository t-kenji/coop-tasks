/** @file       threads.c
 *  @brief      C11 threads like thread component.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2019-02-09 create new.
 *  @copyright  Copyright (c) 2018-2019 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include <semaphore.h>

#include "utils.h"
#include "collections.h"
#include "future.h"
#include "debug.h"
#include "threads.h"

/**
 *  thread_control_block desc.
 */
struct thread_control_block {
    pid_t tid;         /**< tid desc. */
    pid_t ptid;        /**< ptid desc. */
    thrd_t thr;        /**< thr desc. */
    thrd_t pthr;       /**< pthr desc. */
    thrd_start_t func; /**< func desc. */
    void *arg;         /**< arg desc. */
    char name[16];     /**< name desc. */
    promise_t *prms;   /**< prms desc. */
    sem_t *suspend;    /**< suspend desc. */
};

/**
 *  TCB_MAKER desc.
 *
 *  @param  [in]    f   f desc.
 *  @param  [in]    a   a desc.
 *  @param  [in]    p   p desc.
 *  @return Return initialized TCB object.
 */
#define TCB_MAKER(f, a, p)         \
    (struct thread_control_block){ \
        .tid = -1,                 \
        .ptid = gettid(),          \
        .pthr = thrd_current(),    \
        .func = (f),               \
        .arg = (a),                \
        .name = {0},               \
        .prms = (p),               \
        .suspend = NULL,           \
    }

/**
 *  tcbs desc.
 */
static list_t tcbs;

/**
 *  suspends desc.
 */
static mpool_t suspends;

/**
 *  buckets desc.
 */
static mpool_t buckets;

#if 0
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
#endif
static _Thread_local sem_t *suspend;

/**
 *  tcb_initialize desc.
 */
__attribute__((constructor))
static void tcb_initialize(void)
{
    list_create(&tcbs, sizeof(struct thread_control_block), MAX_THREADS);
    mempool_create(&suspends, sizeof(sem_t), MAX_THREADS);
    mempool_create(&buckets, sizeof(struct thread_control_block), 10);
}

/**
 *  tcb_finalizer desc.
 */
__attribute__((destructor))
static void tcb_finalizer(void)
{
    mempool_destroy(&buckets);
    mempool_destroy(&suspends);
    list_destroy(&tcbs);
}

/**
 *  gettid desc.
 *
 *  @return Return own task id.
 */
INLINE pid_t gettid(void)
{
    return syscall(SYS_gettid);
}

INLINE void internal_signaled(int n)
{
    UNUSED_VARIABLE(n);

    sem_wait(suspend);
}

/**
 *  tcb_key desc.
 */
static pthread_key_t tcb_key;

/**
 *  internal_task_finalizer desc.
 *
 *  @param  [in]    arg unused.
 */
INLINE void internal_task_finalizer(void *arg)
{
    UNUSED_VARIABLE(arg);

    struct thread_control_block tcb;
    list_delete(&tcbs, thrd_current(), &tcb);

    mempool_free(&suspends, tcb.suspend);
    pthread_key_delete(tcb_key);
}

/**
 *  internal_entry desc.
 *
 *  @param  [in]    arg arg desc.
 *  @return Returns thread_control_block::func result, NULL if failed.
 */
INLINE void *internal_entry(void *arg)
{
    struct thread_control_block *tcb = (struct thread_control_block *)arg;
    tcb->tid = gettid();
    tcb->thr = thrd_current();
    tcb->suspend = mempool_alloc(&suspends);
    if (tcb->suspend == NULL) {
        ERROR("threads: Can't allocate TCB");
        mempool_free(&buckets, tcb);
        promise_set_value(tcb->prms, -ENOMEM);
        return NULL;
    }
    if (sem_init(tcb->suspend, 0, 0) != 0) {
        ERROR("threads: Can't initialize TCB");
        mempool_free(&buckets, tcb);
        promise_set_value(tcb->prms, -ENOMEM);
        return NULL;
    }
    int ret = list_insert(&tcbs, tcb->thr, tcb);
    if (ret != 0) {
        ERROR("threads: Can't allocate TCB");
        mempool_free(&buckets, tcb);
        promise_set_value(tcb->prms, -ENOMEM);
        return NULL;
    }
    pthread_key_create(&tcb_key, internal_task_finalizer);
    pthread_setspecific(tcb_key, (void *)(intptr_t)tcb->tid);
    mempool_free(&buckets, tcb);

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = internal_signaled;
    act.sa_flags = SA_RESTART | SA_ONSTACK;
    sigemptyset(&act.sa_mask);
    sigaction(SIGURG, &act, NULL);
    suspend = tcb->suspend;

    promise_set_value(tcb->prms, 0);
    tcb->prms = NULL;
    return (void *)(intptr_t)tcb->func(tcb->arg);
}

/**
 *  @details    thrd_create desc.
 *
 *  @param      [out]   thr     thr desc.
 *  @param      [in]    func    func desc.
 *  @param      [in]    arg     arg desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
    if ((thr == NULL) || (func == NULL)) {
        errno = EINVAL;
        return -1;
    }

    promise_t prms = PROMISE_INITIALIZER;
    future_t *ftr = promise_get_future(&prms);

    struct thread_control_block *tcb = mempool_alloc(&buckets);
    if (tcb == NULL) {
        return -1;
    }
    *tcb = TCB_MAKER(func, arg, &prms);

    int err = pthread_create(thr, NULL, internal_entry, tcb);
    if (err != 0) {
        errno = err;
        return -1;
    }
    intmax_t status;
    future_get_value(ftr, &status);
    if (status < 0) {
        errno = status;
        return -1;
    }

    return 0;
}

/**
 *  @details    thrd_current desc.
 *
 *  @return     Return own thread id.
 */
thrd_t thrd_current(void)
{
    return pthread_self();
}

/**
 *  @details    thrd_detach desc.
 *
 *  @param      [in]    thr thr desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int thrd_detach(thrd_t thr)
{
    int err = pthread_detach(thr);
    if (err != 0) {
        errno = err;
        return -1;
    }

    return 0;
}

/**
 *  @details    thrd_equal desc.
 *
 *  @return     Returns true if thr0 and thr1 are same, false if otherwise.
 */
int thrd_equal(thrd_t thr0, thrd_t thr1)
{
    return pthread_equal(thr0, thr1);
}

/**
 *  @details    thrd_exit desc.
 *
 *  @param      [in]    res res desc.
 *  @return     This function does not return.
 */
void thrd_exit(int res)
{
    pthread_exit((void *)(intptr_t)res);
}

/**
 *  @details    thrd_join desc.
 *
 *  @param      [in]    thr thr desc.
 *  @param      [in]    res res desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int thrd_join(thrd_t thr, int *res)
{
    void *status;
    int err = pthread_join(thr, &status);
    if (err != 0) {
        errno = err;
        return -1;
    }

    if (res != NULL) {
        *res = (intptr_t)status;
    }

    return 0;
}

/**
 *  @details    thrd_sleep desc.
 *
 *  @param      [in]    ts  ts desc.
 */
void thrd_sleep(struct timespec *ts)
{
    if (ts == NULL) {
        errno = EINVAL;
        return;
    }

    struct timespec rem = *ts;
    int ret;
    do {
        *ts = rem;
        ret = clock_nanosleep(CLOCK_MONOTONIC, 0, ts, &rem);
    } while (ret == EINTR);
}

/**
 *  @details    thrd_yield desc.
 */
void thrd_yield(void)
{
    sched_yield();
}

/**
 *  @details    thrd_suspend desc.
 *
 *  @param      [in]    thr thr desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int thrd_suspend(thrd_t thr)
{
#if 0
    struct thread_control_block tcb;
    if (list_search(&tcbs, thr, &tcb) != 0) {
        errno = ENOENT;
        return -1;
    }

    int err;
    lock (&mtx) {
        suspend = tcb.suspend;
        err = pthread_kill(thr, SIGURG);
        /* FIXME: Necessary to wait until signal processing is completed. */
    }
    if (err != 0) {
        errno = err;
        return -1;
    }
#else
    int err = pthread_kill(thr, SIGURG);
    if (err != 0) {
        errno = err;
        return -1;
    }
#endif

    return 0;
}

/**
 *  @details    thrd_resume desc.
 *
 *  @param      [in]    thr thr desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int thrd_resume(thrd_t thr)
{
    struct thread_control_block tcb;
    if (list_search(&tcbs, thr, &tcb) != 0) {
        errno = ENOENT;
        return -1;
    }

    sem_post(tcb.suspend);

    return 0;
}

/**
 *  @details    thrd_cancel desc.
 *
 *  @param      [in]    thr thr desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int thrd_cancel(thrd_t thr)
{
    int err = pthread_cancel(thr);
    if (err != 0) {
        errno = err;
        return -1;
    }

    return 0;
}

/**
 *  @details    thrd_raise desc.
 *
 *  @param      [in]    sig sig desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int thrd_raise(int sig)
{
    return raise(sig);
}

/**
 *  @details    thrd_kill desc.
 *
 *  @param      [in]    thr thr desc.
 *  @param      [in]    sig sig desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int thrd_kill(thrd_t thr, int sig)
{
    int err = pthread_kill(thr, sig);
    if (err != 0) {
        errno = err;
        return -1;
    }

    return 0;
}

/**
 *  @details    thrd_set_name desc.
 *
 *  @param      [in]    thr     thr desc.
 *  @param      [in]    name    name desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int thrd_set_name(thrd_t thr, const char *name)
{
    struct thread_control_block tcb;
    if (list_search(&tcbs, thr, &tcb) != 0) {
        errno = ENOENT;
        return -1;
    }

    strncpy(tcb.name, name, sizeof(tcb.name) - 1);
    pthread_setname_np(tcb.thr, tcb.name);

    if (list_update(&tcbs, thr, &tcb) != 0) {
        errno = EBADE;
        return -1;
    }

    return 0;
}

/**
 *  @details    thrd_get_name desc.
 *
 *  @param      [in]    thr     thr desc.
 *  @param      [out]   name    name desc.
 *  @param      [in]    len     len desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int thrd_get_name(thrd_t thr, char *name, size_t len)
{
    struct thread_control_block tcb;
    if (list_search(&tcbs, thr, &tcb) != 0) {
        errno = ENOENT;
        return -1;
    }

    strncpy(name, tcb.name, len);

    return 0;
}


int thrd_set_prior(thrd_t thr, int prior)
{
    UNUSED_VARIABLE(thr);
    UNUSED_VARIABLE(prior);
    errno = ENOTSUP;
    return -1;
}

int thrd_get_prior(thrd_t thr)
{
    UNUSED_VARIABLE(thr);
    errno = ENOTSUP;
    return -1;
}

int thrd_set_affinity(thrd_t thr, const cpu_set_t *cpuset)
{
    UNUSED_VARIABLE(thr);
    UNUSED_VARIABLE(cpuset);
    errno = ENOTSUP;
    return -1;
}

int thrd_get_affinity(thrd_t thr, cpu_set_t *cpuset)
{
    UNUSED_VARIABLE(thr);
    UNUSED_VARIABLE(cpuset);
    errno = ENOTSUP;
    return -1;
}
