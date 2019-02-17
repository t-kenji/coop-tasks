/** @file       future.h
 *  @brief      Future / Promise pattern component.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2019-02-17 create new.
 *  @copyright  Copyright (c) 2018-2019 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#ifndef __TASKS_EXPORT_FUTURE_H__
#define __TASKS_EXPORT_FUTURE_H__

#if defined(__cplusplus)
extern "C" {
#endif

/**
 *  future desc.
 */
typedef struct future {
    pthread_mutex_t mtx; /**< mtx desc. */
    pthread_cond_t cnd;  /**< cnd desc. */
    bool done;           /**< done desc. */
    intmax_t value;      /**< value desc. */
} future_t;

/**
 *  FUTURE_INITIALIZER desc.
 */
#define FUTURE_INITIALIZER                \
    {                                     \
        .mtx = PTHREAD_MUTEX_INITIALIZER, \
        .cnd = PTHREAD_COND_INITIALIZER,  \
        .done = false,                    \
        .value = 0,                       \
    }

/**
 *  promise desc.
 */
typedef struct promise {
    future_t ftr;         /**< ftr desc. */
    pthread_mutex_t *mtx; /**< mtx desc. */
    pthread_cond_t *cnd;  /**< cnd desc. */
    bool *done;           /**< done desc. */
    intmax_t *value;      /**< value desc. */
} promise_t;

/**
 *  PROMISE_INITIALIZER desc.
 */
#define PROMISE_INITIALIZER        \
    {                              \
        .ftr = FUTURE_INITIALIZER, \
        .mtx = NULL,               \
        .cnd = NULL,               \
        .done = NULL,              \
        .value = NULL,             \
    }

/**
 *  promise_init summary.
 */
int promise_init(promise_t *prms);

/**
 *  promise_get_future summary.
 */
future_t *promise_get_future(promise_t *prms);

/**
 *  promise_set_value summary.
 */
int promise_set_value(promise_t *prms, intmax_t value);

/**
 *  future_has_value summary.
 */
bool future_has_value(future_t *ftr);

/**
 *  future_get_value summary.
 */
int future_get_value(future_t *ftr, intmax_t *value);

#if defined(__cplusplus)
}
#endif

#endif /* __TASKS_EXPORT_FUTURE_H__ */
