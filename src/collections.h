/** @file       collections.h
 *  @brief      Generic collections.
 *
 *              Provide some collection (e.g. list, queue).
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2019-02-03 create new.
 *  @copyright  Copyright (c) 2018-2019 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#ifndef __TASKS_INTERNAL_COLLECTIONS_H__
#define __TASKS_INTERNAL_COLLECTIONS_H__

#if defined(__cplusplus)
#include <atomic>
#undef _Atomic
#define _Atomic(T) std::atomic<T>

extern "C" {
#else
#include <stdalign.h>
#include <stdatomic.h>
#endif

#define MEMPOOL_IMPLEMENTED_QUEUE

struct memory_fragment;

/**
 *  memory_node desc.
 */
struct memory_node {
    uint32_t count;               /**< count desc. */
    struct memory_fragment *frag; /**< frag desc. */
};

/**
 *  memory_pool desc.
 */
typedef struct memory_pool {
    void *pool;                                   /**< pool desc. */
    size_t data_bytes;                            /**< data_bytes desc. */
    size_t capacity;                              /**< capacity desc. */
    _Atomic(size_t) freeable;                     /**< freeable desc. */
    alignas(16) _Atomic(struct memory_node) head; /**< head desc. */
    alignas(16) _Atomic(struct memory_node) tail; /**< tail desc. */
} mpool_t;

/**
 *  MEMORY_POOL_INITIALIZER desc.
 */
#define MEMORY_POOL_INITIALIZER \
    {                           \
        .pool = NULL,           \
        .data_bytes = 0,        \
        .capacity = 0,          \
        .freeable = 0,          \
        .head = {               \
            .count = 0,         \
            .frag = NULL,       \
        },                      \
        .tail = {               \
            .count = 0,         \
            .frag = NULL,       \
        },                      \
    }

/**
 *  mempool_create summary.
 */
int mempool_create(mpool_t *mp, size_t data_bytes, size_t capacity);

/**
 *  mempool_destroy summary.
 */
int mempool_destroy(mpool_t *mp);

/**
 *  mempool_clear summary.
 */
int mempool_clear(mpool_t *mp);

/**
 *  mempool_alloc summary.
 */
void *mempool_alloc(mpool_t *mp);

/**
 *  mempool_free summary.
 */
void mempool_free(mpool_t *mp, void *ptr);

/**
 *  mempool_data_bytes summary.
 */
ssize_t mempool_data_bytes(mpool_t *mp);

/**
 *  mempool_capacity summary.
 */
ssize_t mempool_capacity(mpool_t *mp);

/**
 *  mempool_freeable summary.
 */
ssize_t mempool_freeable(mpool_t *mp);

/**
 *  mempool_contains summary.
 */
bool mempool_contains(mpool_t *mp, const void *ptr);

struct queue_value;

/**
 *  queue_node desc.
 */
struct queue_node {
    uint32_t count;          /**< count desc. */
    struct queue_value *val; /**< val desc. */
};

/**
 *  queue desc.
 */
typedef struct queue {
    mpool_t pool;                                /**< pool desc. */
    size_t val_bytes;                            /**< val_bytes desc. */
    alignas(16) _Atomic(struct queue_node) head; /**< head desc. */
    alignas(16) _Atomic(struct queue_node) tail; /**< tail desc. */
} que_t;

/**
 *  queue_create summary.
 */
int queue_create(que_t *q, size_t val_bytes, size_t capacity);

/**
 *  queue_destroy summary.
 */
int queue_destroy(que_t *q);

/**
 *  queue_enqueue summary.
 */
int queue_enqueue(que_t *q, const void *val);

/**
 *  queue_dequeue summary.
 */
int queue_dequeue(que_t *q, void *val);

typedef struct deque {
} deq_t;

int deque_create(deq_t *q, size_t val_bytes, size_t capacity);
int deque_destroy(deq_t *q);
int deque_push(deq_t *q, const void *val);
int deque_pop(deq_t *q, void *val);
int deque_shift(deq_t *q, const void *val);
int deque_unshift(deq_t *q, void *val);

/**
 *  lkey_t desc.
 */
typedef intptr_t lkey_t;

struct list_node;

/**
 *  list_t desc.
 */
typedef struct list {
    mpool_t pool;           /**< pool desc. */
    size_t val_bytes;       /**< val_bytes desc. */
    struct list_node *head; /**< head desc. */
    struct list_node *tail; /**< tail desc. */
} list_t;

/**
 *  list_create summary.
 */
int list_create(list_t *lst, size_t val_bytes, size_t capacity);

/**
 *  list_destroy summary.
 */
int list_destroy(list_t *lst);

/**
 *  list_insert summary.
 */
int list_insert(list_t *lst, lkey_t key, const void *val);

/**
 *  list_delete summary.
 */
int list_delete(list_t *lst, lkey_t key, void *val);

/**
 *  list_search summary.
 */
int list_search(list_t *lst, lkey_t key, void *val);

/**
 *  list_update summary.
 */
int list_update(list_t *lst, lkey_t key, const void *val);

/**
 *  list_dump summary.
 */
void list_dump(list_t *lst);

#if defined(__cplusplus)
}
#endif

#endif /* __TASKS_INTERNAL_COLLECTIONS_H__ */
