/** @file       future.c
 *  @brief      Future / Promise pattern component.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2019-02-17 create new.
 *  @copyright  Copyright (c) 2018-2019 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#include "utils.h"
#include "threads.h"
#include "debug.h"
#include "future.h"

/**
 *  @details    promise_init desc.
 *
 *  @param      [out]   prms    prms desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int promise_init(promise_t *prms)
{
    if (prms == NULL) {
        errno = EINVAL;
        return -1;
    }

    *prms = (promise_t)PROMISE_INITIALIZER;

    return 0;
}

/**
 *  @details    promise_get_future desc.
 *
 *  @param      [in,out]    prms    prms desc.
 *  @return     Returns future object if succeed, NULL if failed.
 */
future_t *promise_get_future(promise_t *prms)
{
    if (prms == NULL) {
        errno = EINVAL;
        return NULL;
    }

    prms->mtx = &prms->ftr.mtx;
    prms->cnd = &prms->ftr.cnd;
    prms->done = &prms->ftr.done;
    prms->value = &prms->ftr.value;

    return &prms->ftr;
}

/**
 *  @details    promise_set_value desc.
 *
 *  @param      [in,out]    prms    prms desc.
 *  @param      [in]        value   value desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int promise_set_value(promise_t *prms, intmax_t value)
{
    if (prms == NULL) {
        errno = EINVAL;
        return -1;
    }

    pthread_mutex_lock(prms->mtx);
    *prms->value = value;
    *prms->done = true;
    pthread_cond_signal(prms->cnd);
    pthread_mutex_unlock(prms->mtx);

    return 0;
}

/**
 *  @details    future_has_value desc.
 *
 *  @param      [in]    ftr ftr desc.
 *  @return     Returns true if has value, false if otherwise.
 */
bool future_has_value(future_t *ftr)
{
    if (ftr == NULL) {
        errno = EINVAL;
        return false;
    }

    pthread_mutex_lock(&ftr->mtx);
    bool has_value = ftr->done;
    pthread_mutex_unlock(&ftr->mtx);

    return has_value;
}

/**
 *  @details    future_get_value desc.
 *
 *  @param      [in]    ftr     ftr desc.
 *  @param      [out]   value   value desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int future_get_value(future_t *ftr, intmax_t *value)
{
    if (ftr == NULL) {
        errno = EINVAL;
        return -1;
    }

    pthread_mutex_lock(&ftr->mtx);
    while (!ftr->done) {
        pthread_cond_wait(&ftr->cnd, &ftr->mtx);
    }
    pthread_mutex_unlock(&ftr->mtx);

    if (value != NULL) {
        *value = ftr->value;
    }

    return 0;
}
