/** @file       collections.c
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
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#include "utils.h"
#include "debug.h"
#include "collections.h"

/**
 *  memory_fragment desc.
 */
struct memory_fragment {
    struct memory_node next; /**< next desc. */
    uint8_t data[];          /**< data desc. */
};

/**
 *  MEMORY_FRAGMENT_MAKER desc.
 *
 *  @return Return initialized #memory_fragment object.
 */
#define MEMORY_FRAGMENT_MAKER() \
    (struct memory_fragment){   \
        .next = {               \
            .count = 0,         \
            .frag = NULL,       \
        },                      \
    }

/**
 *  MEMORY_POOL_MAKER desc.
 *
 *  @param  [in]    p   p desc.
 *  @param  [in]    b   b desc.
 *  @param  [in]    c   c desc.
 *  @return Return initialized #memory_pool object.
 */
#define MEMORY_POOL_MAKER(p, b, c) \
    (struct memory_pool){          \
        .pool = (p),               \
        .data_bytes = (b),         \
        .capacity = (c),           \
        .freeable = 0,             \
        .head = {                  \
            .count = 0,            \
            .frag = NULL,          \
        },                         \
        .tail = {                  \
            .count = 0,            \
            .frag = NULL,          \
        },                         \
    }

/**
 *  max desc.
 *
 *  @param  [in]    a   a desc.
 *  @param  [in]    b   b desc.
 *  @return Returns larger of @c a and @c b.
 */
#define max(a, b) (((a) > (b)) ? (a) : (b))

/**
 *  memory_node_equals desc.
 *
 *  @param  [in]    a   a desc.
 *  @param  [in]    b   b desc.
 *  @return Returns true if @c a and @c b are same, false if otherwise.
 */
static inline bool memory_node_equals(struct memory_node a, struct memory_node b)
{
    return (a.count == b.count) && (a.frag == b.frag);
}

/**
 *  queue_node_equals desc.
 *
 *  @param  [in]    a   a desc.
 *  @param  [in]    b   b desc.
 *  @return Returns true if @c a and @c b are same, false if otherwise.
 */
static inline bool queue_node_equals(struct queue_node a, struct queue_node b)
{
    return (a.count == b.count) && (a.val == b.val);
}

/**
 *  equals desc.
 *
 *  @param  [in]    a   a desc.
 *  @param  [in]    b   b desc.
 *  @return Returns true if @c a and @c b are same, false if otherwise.
 */
#define equals(a, b)                            \
    _Generic((a),                               \
        struct memory_node: memory_node_equals, \
         struct queue_node: queue_node_equals   \
    )(a, b)

/**
 *  internal_mempool_put desc.
 *
 *  @param  [in]    self    self desc.
 *  @param  [in]    frag    frag desc.
 */
INLINE void internal_mempool_put(struct memory_pool *self, struct memory_fragment *frag)
{
#if defined(MEMPOOL_IMPLEMENTED_QUEUE)
    *frag = MEMORY_FRAGMENT_MAKER();

    struct memory_node tail, tmp;
    while (true) {
        tail = atomic_load(&self->tail);
        struct memory_node next = tail.frag->next;

        if (equals(tail, self->tail)) {
            if (next.frag == NULL) {
                tmp.frag = frag;
                tmp.count = next.count + 1;
                if (atomic_compare_exchange_weak(&tail.frag->next, &next, tmp)) {
                    break;
                }
            } else {
                tmp.frag = next.frag;
                tmp.count = tail.count + 1;
                atomic_compare_exchange_weak(&self->tail, &tail, tmp);
            }
        }
    }
    tmp.frag = frag;
    tmp.count = tail.count + 1;
    atomic_compare_exchange_weak(&self->tail, &tail, tmp);
    atomic_fetch_add(&self->freeable, 1);
#else
    struct memory_node next, orig = atomic_load(&self->head);
    do {
        node->next.frag = orig.frag;
        next.frag = frag;
        next.count = orig.count + 1;
    } while (!atomic_compare_exchange_weak(&self->head, &orig, next));
    atomic_fetch_add(&self->freeable, 1);
#endif
}

/**
 *  internal_mempool_pick desc.
 *
 *  @param  [in,out]    self    self desc.
 *  @return Returns pooled memory if succeed, NULL if failed.
 */
INLINE struct memory_fragment *internal_mempool_pick(struct memory_pool *self)
{
#if defined(MEMPOOL_IMPLEMENTED_QUEUE)
    struct memory_node head;
    while (true) {
        head = atomic_load(&self->head);
        struct memory_node tail = atomic_load(&self->tail),
                           next = head.frag->next,
                           tmp;

        if (equals(head, self->head)) {
            if (head.frag == tail.frag) {
                if (next.frag == NULL) {
                    errno = ENOMEM;
                    return NULL;
                }
                tmp.frag = next.frag;
                tmp.count = tail.count + 1;
                atomic_compare_exchange_weak(&self->tail, &tail, tmp);
            } else {
                tmp.frag = next.frag;
                tmp.count = head.count + 1;
                if (atomic_compare_exchange_weak(&self->head, &head, tmp)) {
                    break;
                }
            }
        }
    }
    atomic_fetch_sub(&self->freeable, 1);

    return head.frag;
#else
    struct memory_node next, orig = atomic_load(&self->head);
    do {
        if (orig.frag == NULL) {
            errno = ENOMEM;
            return NULL;
        }
        next.frag = orig.frag->next.frag;
        next.count = orig.count + 1;
    } while (!atomic_compare_exchange_weak(&self->head, &orig, next));
    atomic_fetch_sub(&self->freeable, 1);

    return orig.frag;
#endif
}

/**
 *  internal_mempool_aligned_data_bytes desc.
 *
 *  @param  [in,out]    self    self desc.
 *  @return Return memory fragment size.
 */
INLINE size_t internal_mempool_aligned_data_bytes(struct memory_pool *self)
{
    size_t frag_bytes = max(self->data_bytes, sizeof(*self->head.frag));
    /* Workarround: SEGV at atomic operations. */
    if (frag_bytes % 16) {
        frag_bytes += 16 - (frag_bytes % 16);
    }
    return frag_bytes;
}

/**
 *  internal_mempool_setup desc.
 *
 *  @param  [in,out]    self        self desc.
 *  @param  [in]        pool        pool desc.
 *  @param  [in]        data_bytes  data_bytes desc.
 *  @param  [in]        capacity    capacity desc.
 */
INLINE void internal_mempool_setup(struct memory_pool *self,
                                   void *pool,
                                   size_t data_bytes,
                                   size_t capacity)
{
    *self = MEMORY_POOL_MAKER(pool, data_bytes, capacity);

#if defined(MEMPOOL_IMPLEMENTED_QUEUE)
    struct memory_node node = {
        .count = 0,
        .frag = (struct memory_fragment *)pool,
    };
    *node.frag = MEMORY_FRAGMENT_MAKER();
    atomic_store(&self->head, node);
    atomic_store(&self->tail, node);

    size_t frag_bytes = internal_mempool_aligned_data_bytes(self);
    struct memory_fragment *frag;
    for (size_t i = 1; i < self->capacity + 1; ++i) {
        frag = (struct memory_fragment *)((uintptr_t)pool + (frag_bytes * i));
        internal_mempool_put(self, frag);
    }
#else
    size_t frag_bytes = internal_mempool_aligned_data_bytes(self);
    for (size_t i = 0; i < self->capacity; ++i) {
        struct memory_fragment *frag = (struct memory_fragment *)((uintptr_t)pool + (frag_bytes * i));
        *frag = MEMORY_MAKER();
        internal_mempool_put(self, frag);
    }
#endif
}

/**
 *  @details    mempool_create desc.
 *
 *  @param      [out]   mp          mp desc.
 *  @param      [in]    data_bytes  data_bytes desc.
 *  @param      [in]    capacity    capacity desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int mempool_create(mpool_t *mp, size_t data_bytes, size_t capacity)
{
    if ((mp == NULL) || (data_bytes == 0) || (capacity == 0)) {
        errno = EINVAL;
        return -1;
    }

    SELFLIZE(struct memory_pool *, mp);

    *self = MEMORY_POOL_MAKER(NULL, data_bytes, capacity);
    size_t frag_bytes = internal_mempool_aligned_data_bytes(self);
#if defined(MEMPOOL_IMPLEMENTED_QUEUE)
    void *pool = calloc(capacity + 1, frag_bytes);
#else
    void *pool = calloc(capacity, frag_bytes);
#endif
    if (pool == NULL) {
        return -1;
    }

    internal_mempool_setup(self, pool, data_bytes, capacity);

    return 0;
}

/**
 *  @details    mempool_destroy desc.
 *
 *  @param      [in,out]    mp  mp desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int mempool_destroy(mpool_t *mp)
{
    if (mp == NULL) {
        errno = EINVAL;
        return -1;
    }

    SELFLIZE(struct memory_pool *, mp);

    free(self->pool);
    self->pool = NULL;

    return 0;
}

/**
 *  @details    mempool_clear desc.
 *
 *  @param      [in]    mp  mp desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int mempool_clear(mpool_t *mp)
{
    if (mp == NULL) {
        errno = EINVAL;
        return -1;
    }

    SELFLIZE(struct memory_pool *, mp);

    internal_mempool_setup(self, self->pool, self->data_bytes, self->capacity);

    return 0;
}

/**
 *  @details    mempool_alloc desc.
 *
 *  @param      [in,out]    mp  mp desc.
 *  @return     Returns pooled memory if succeed, NULL if failed.
 */
void *mempool_alloc(mpool_t *mp)
{
    if (mp == NULL) {
        errno = EINVAL;
        return NULL;
    }

    SELFLIZE(struct memory_pool *, mp);

    return internal_mempool_pick(self);
}

/**
 *  @details    mempool_free desc.
 *
 *  @param      [in,out]    mp  mp desc.
 *  @param      [in]        ptr ptr desc.
 */
void mempool_free(mpool_t *mp, void *ptr)
{
    if ((mp == NULL) || (ptr == NULL)) {
        return;
    }

    SELFLIZE(struct memory_pool *, mp);

    internal_mempool_put(self, ptr);
}

/**
 *  @details    mempool_data_bytes desc.
 *
 *  @param      [in]    mp  mp desc.
 *  @return     Returns data size if succeed, -1 if failed.
 */
ssize_t mempool_data_bytes(mpool_t *mp)
{
    if (mp == NULL) {
        errno = EINVAL;
        return -1;
    }

    SELFLIZE(struct memory_pool *, mp);

    return self->data_bytes;
}

/**
 *  @details    mempool_capacity desc.
 *
 *  @param      [in]    mp  mp desc.
 *  @return     Returns capacity if succeed, -1 if failed.
 */
ssize_t mempool_capacity(mpool_t *mp)
{
    if (mp == NULL) {
        errno = EINVAL;
        return -1;
    }

    SELFLIZE(struct memory_pool *, mp);

    return self->capacity;
}

/**
 *  @details    mempool_freeable desc.
 *
 *  @param      [in]    mp  mp desc.
 *  @return     Returns freeable number if succeed, -1 if failed.
 */
ssize_t mempool_freeable(mpool_t *mp)
{
    if (mp == NULL) {
        errno = EINVAL;
        return -1;
    }

    SELFLIZE(struct memory_pool *, mp);

    return atomic_load(&self->freeable);
}

/**
 *  @details    mempool_contains desc.
 *
 *  @param      [in]    mp  mp desc.
 *  @param      [in]    ptr ptr desc.
 *  @return     Returns true if @c mp contains @c ptr, false if otherwise.
 */
bool mempool_contains(mpool_t *mp, const void *ptr)
{
    if (mp == NULL) {
        errno = EINVAL;
        return false;
    }

    SELFLIZE(struct memory_pool *, mp);

    size_t frag_bytes = internal_mempool_aligned_data_bytes(self);
#if defined(MEMPOOL_IMPLEMENTED_QUEUE)
    size_t pool_size = frag_bytes * self->capacity + 1;
#else
    size_t pool_size = frag_bytes * self->capacity;
#endif
    void *pool_end = (void *)((uintptr_t)self->pool + pool_size);
    return (self->pool <= ptr) && (ptr < pool_end);
}

/**
 *  queue_value desc.
 */
struct queue_value {
    struct queue_node next; /**< next desc. */
    uint8_t data[];         /**< data desc. */
};

/**
 *  QUEUE_VALUE_MAKER desc.
 *
 *  @return Return initialized #queue_value object.
 */
#define QUEUE_VALUE_MAKER() \
    (struct queue_value){   \
        .next = {           \
            .count = 0,     \
            .val = NULL,    \
        },                  \
    }

/**
 *  queue_value desc.
 *
 *  @param  [in,out]    self    self desc.
 *  @param  [in]        val     val desc.
 *  @return Returns value object if succeed, NULL if failed.
 */
INLINE struct queue_value *queue_alloc_value(struct queue *self, const void *val)
{
    struct queue_value *v = mempool_alloc(&self->pool);
    if (v == NULL) {
        return NULL;
    }

    *v = QUEUE_VALUE_MAKER();
    if (val != NULL) {
        memcpy(v->data, val, self->val_bytes);
    }

    return v;
}

/**
 *  @details    queue_create desc.
 *
 *  @param      [out]   q           q desc.
 *  @param      [in]    val_bytes   val_bytes decs.
 *  @param      [in]    capacity    capacity desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int queue_create(que_t *q, size_t val_bytes, size_t capacity)
{
    if ((q == NULL) || (val_bytes == 0) || (capacity == 0)) {
        errno = EINVAL;
        return -1;
    }

    SELFLIZE(struct queue *, q);

    size_t node_bytes = sizeof(*self->head.val) + val_bytes;
    int ret = mempool_create(&self->pool, node_bytes, capacity + 1);
    if (ret != 0) {
        return -1;
    }
    self->val_bytes = val_bytes;

    struct queue_value *val = queue_alloc_value(self, NULL);
    struct queue_node node = {
        .count = 0,
        .val = val,
    };
    atomic_init(&self->head, node);
    atomic_init(&self->tail, node);

    return 0;
}

/**
 *  @details    queue_destroy desc.
 *
 *  @param      [in,out]    q   q desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int queue_destroy(que_t *q)
{
    if (q == NULL) {
        errno = EINVAL;
        return -1;
    }

    SELFLIZE(struct queue *, q);

    mempool_destroy(&self->pool);

    return 0;
}

/**
 *  @details    queue_enqueue desc.
 *
 *  @param      [in,out]    q   q desc.
 *  @param      [in]        val val desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int queue_enqueue(que_t *q, const void *val)
{
    if ((q == NULL) || (val == NULL)) {
        errno = EINVAL;
        return -1;
    }

    SELFLIZE(struct queue *, q);

    struct queue_value *v = queue_alloc_value(self, val);
    if (v == NULL) {
        return -1;
    }

    struct queue_node tail, tmp;
    while (true) {
        tail = atomic_load(&self->tail);
        struct queue_node next = tail.val->next;

        if (equals(tail, self->tail)) {
            if (next.val == NULL) {
                tmp.val = v;
                tmp.count = next.count + 1;
                if (atomic_compare_exchange_weak(&tail.val->next, &next, tmp)) {
                    break;
                }
            } else {
                tmp.val = next.val;
                tmp.count = tail.count + 1;
                atomic_compare_exchange_weak(&self->tail, &tail, tmp);
            }
        }
    }
    tmp.val = v;
    tmp.count = tail.count + 1;
    atomic_compare_exchange_weak(&self->tail, &tail, tmp);

    return 0;
}

/**
 *  @details    queue_dequeue desc.
 *
 *  @param      [in]    q   q desc.
 *  @param      [out]   val val desc.
 *  @return     Returns zero if succeed, -1 if faild.
 */
int queue_dequeue(que_t *q, void *val)
{
    if (q == NULL) {
        errno = EINVAL;
        return -1;
    }

    SELFLIZE(struct queue *, q);

    struct queue_node head;
    while (true) {
        head = atomic_load(&self->head);
        struct queue_node tail = atomic_load(&self->tail),
                          next = head.val->next,
                          tmp;

        if (equals(head, self->head)) {
            if (head.val == tail.val) {
                if (next.val == NULL) {
                    errno = ENOENT;
                    return -1;
                }
                tmp.val = next.val;
                tmp.count = tail.count + 1;
                atomic_compare_exchange_weak(&self->tail, &tail, tmp);
            } else {
                if (val != NULL) {
                    memcpy(val, next.val->data, self->val_bytes);
                }
                tmp.val= next.val;
                tmp.count = head.count + 1;
                if (atomic_compare_exchange_weak(&self->head, &head, tmp)) {
                    break;
                }
            }
        }
    }

    mempool_free(&self->pool, head.val);

    return 0;
}

int deque_create(deq_t *q, size_t val_bytes, size_t capacity)
{
    return -1;
}

int deque_destroy(deq_t *q)
{
    return -1;
}

int deque_push(deq_t *q, const void *val)
{
    return -1;
}

int deque_pop(deq_t *q, void *val)
{
    return -1;
}

int deque_shift(deq_t *q, const void *val)
{
    return -1;
}

int deque_unshift(deq_t *q, void *val)
{
    return -1;
}

enum {
    MARKED = 0x00000001,      /**< MARKED desc. */
    UNMARKED = 0x00000000,    /**< UNMARKED desc. */
    MARKED_MASK = 0x00000001, /**< MARKED_MASK desc. */

    FLAGGED = 0x00000002,      /**< FLAGGED desc. */
    UNFLAGGED = 0x00000000,    /**< UNFLAGGED desc. */
    FLAGGED_MASK = 0x00000002, /**< FLAGGED_MASK desc. */
};

/**
 *  list_node_ref desc.
 */
struct list_node_ref {
    intptr_t mark;          /**< mark desc. */
    struct list_node *node; /**< node desc. */
};

/**
 *  list_node desc.
 */
struct list_node {
    alignas(16) _Atomic(struct list_node_ref) succ; /**< succ desc. */
    struct list_node *backlink;                     /**< backlink desc. */
    lkey_t key;                                     /**< key desc. */
    uint8_t val[];                                  /**< val desc. */
};

/**
 *  is_marked desc.
 *
 *  @param  [in]    ref ref desc.
 *  @return Returns true if marked, false if unmarked.
 */
#define is_marked(ref)   ((intptr_t)(ref.mark & MARKED_MASK) == (intptr_t)MARKED)

/**
 *  is_marked_atomic desc.
 *
 *  @param  [in]    ref ref desc.
 *  @return Returns true if marked, false if unmarked.
 */
#define is_marked_atomic(ref)   ((intptr_t)(atomic_load(&ref).mark & MARKED_MASK) == (intptr_t)MARKED)

/**
 *  is_flagged desc.
 *
 *  @param  [in]    ref ref desc.
 *  @return Returns true if flagged, false if unflagged.
 */
#define is_flagged(ref)   ((intptr_t)(ref.mark & FLAGGED_MASK) == (intptr_t)FLAGGED)

/**
 *  is_flagged_atomic desc.
 *
 *  @param  [in]    ref ref desc.
 *  @return Returns true if flagged, false if unflagged.
 */
#define is_flagged_atomic(ref)   ((intptr_t)(atomic_load(&ref).mark & FLAGGED_MASK) == (intptr_t)FLAGGED)

/**
 *  make_ref desc.
 *
 *  @param  [in]    node    node desc.
 *  @param  [in]    marked  marked desc.
 *  @param  [in]    flagged flagged desc.
 *  @return Return #list_node_ref object.
 */
INLINE struct list_node_ref make_ref(const struct list_node *node, intptr_t marked, intptr_t flagged)
{
    return (struct list_node_ref){
        .mark = (marked | flagged),
        .node = (struct list_node *)node,
    };
}

/**
 *  get_marked_ref desc.
 *
 *  @param  [in]    node    node desc.
 *  @return Return marked and unflagged object.
 */
#define get_marked_ref(node) make_ref(node, MARKED, UNFLAGGED)

/**
 *  get_unmarked_ref desc.
 *
 *  @param  [in]    node    node desc.
 *  @return Return unmarked and unflagged object.
 */
#define get_unmarked_ref(node) make_ref(node, UNMARKED, UNFLAGGED)

/**
 *  get_flagged_ref desc.
 *
 *  @param  [in]    node    node desc.
 *  @return Return unmarked and flagged object.
 */
#define get_flagged_ref(node) make_ref(node, UNMARKED, FLAGGED)

/**
 *  get_unflagged_ref desc.
 *
 *  @param  [in]    node    node desc.
 *  @return Return unmarked and unflagged object.
 */
#define get_unflagged_ref(node) make_ref(node, UNMARKED, UNFLAGGED)

/**
 *  list_alloc_node desc.
 *
 *  @param  [in,out]    self    self desc.
 *  @param  [in]        key     key desc.
 *  @param  [in]        val     val desc.
 *  @return Returns node object if succeed, NULL if failed.
 */
INLINE struct list_node *list_alloc_node(struct list *self, lkey_t key, const void *val)
{
    struct list_node *node = mempool_alloc(&self->pool);
    if (node == NULL) {
        return NULL;
    }

    struct list_node_ref succ = {
        .mark = (intptr_t)(UNMARKED | UNFLAGGED),
        .node = NULL,
    };
    atomic_init(&node->succ, succ);
    node->backlink = NULL;
    node->key = key;
    if (val != NULL) {
        memcpy(node->val, val, self->val_bytes);
    }

    return node;
}

/**
 *  help_marked desc.
 *
 *  @param  [in,out]    prev    prev desc.
 *  @param  [in]        del     del desc.
 */
INLINE void help_marked(struct list_node *prev, const struct list_node *del)
{
    struct list_node_ref exp = get_flagged_ref(del);
    atomic_compare_exchange_weak(&prev->succ,
                                 &exp,
                                 get_unflagged_ref(atomic_load(&del->succ).node));
}

/**
 *  search_from desc.
 *
 *  @param  [in]    key         key desc.
 *  @param  [in]    curr        curr desc.
 *  @param  [out]   curr_ptr    curr_ptr desc.
 *  @param  [out]   next_ptr    next_ptr desc.
 */
STATIC void search_from(lkey_t key,
                        struct list_node *curr,
                        struct list_node **curr_ptr,
                        struct list_node **next_ptr)
{
    struct list_node *next = atomic_load(&curr->succ).node;

    while (next->key <= key) {
        while (is_marked_atomic(next->succ)
               && (! is_marked_atomic(curr->succ)
                   || (atomic_load(&curr->succ).node != next))) {

            if (atomic_load(&curr->succ).node == next) {
                help_marked(curr, next);
            }
            next = atomic_load(&curr->succ).node;
        }

        if (next->key <= key) {
            curr= next;
            next= atomic_load(&curr->succ).node;
        }
    }

    *curr_ptr = curr;
    *next_ptr = next;
}

/**
 *  @details    list_create desc.
 *
 *  @param      [out]   lst         lst desc.
 *  @param      [in]    val_bytes   val_bytes desc.
 *  @param      [in]    capacity    capacity desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int list_create(list_t *lst, size_t val_bytes, size_t capacity)
{
    if ((lst == NULL) || (val_bytes == 0) || (capacity == 0)) {
        errno = EINVAL;
        return -1;
    }

    SELFLIZE(struct list *, lst);

    size_t node_bytes = sizeof(*self->head) + val_bytes;
    int ret = mempool_create(&self->pool, node_bytes, capacity + 2);
    if (ret != 0) {
        return -1;
    }
    self->val_bytes = val_bytes;
    self->head = list_alloc_node(self, INTPTR_MIN, NULL);
    self->tail = list_alloc_node(self, INTPTR_MAX, NULL);
    struct list_node_ref tmp = {
        .mark = (intptr_t)(UNMARKED | UNFLAGGED),
        .node = self->tail,
    };
    atomic_init(&self->head->succ, tmp);

    return 0;
}

/**
 *  @details    list_destroy desc.
 *
 *  @param      [in,out]    lst lst desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int list_destroy(list_t *lst)
{
    if (lst == NULL) {
        errno = EINVAL;
        return -1;
    }

    SELFLIZE(struct list *, lst);

    mempool_destroy(&self->pool);

    return 0;
}

STATIC void help_flagged(struct list_node *prev, struct list_node *del);

/**
 *  try_mark desc.
 *
 *  @param  [in,out]    del del desc.
 */
STATIC void try_mark(struct list_node *del)
{
    do {
        struct list_node *next = atomic_load(&del->succ).node;
        struct list_node_ref exp = get_unmarked_ref(next);

        atomic_compare_exchange_weak(&del->succ,
                                     &exp,
                                     get_marked_ref(next));

        struct list_node_ref result = del->succ;
        if (!is_marked(result) && is_flagged(result)) {
            help_flagged(del, result.node);
        }
    } while (!is_marked_atomic(del->succ));
}

/**
 *  help_flagged desc.
 *
 *  @param  [in,out]    prev    prev desc.
 *  @param  [in,out]    del     del desc.
 */
STATIC void help_flagged(struct list_node *prev, struct list_node *del)
{
    del->backlink = prev;

    if (!is_marked_atomic(del->succ)) {
        try_mark(del);
    }

    help_marked(prev, del);
}

/**
 *  @details    list_insert desc.
 *
 *  @param      [in,out]    lst lst desc.
 *  @param      [in]        key key desc.
 *  @param      [in]        val val desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int list_insert(list_t *lst, lkey_t key, const void *val)
{
    SELFLIZE(struct list *, lst);

    struct list_node *prev, *next;

    search_from(key, self->head, &prev, &next);

    if (prev->key == key) {
        errno = EEXIST;
        return -1;
    }

    struct list_node *node = list_alloc_node(self, key, val);
    if (node == NULL) {
        return -1;
    }

    while (true) {
        struct list_node_ref prev_succ = atomic_load(&prev->succ);

        if (is_flagged(prev_succ)) {
            help_flagged(prev, prev_succ.node);
        } else {
            node->succ = make_ref(next, UNMARKED, UNFLAGGED);
            struct list_node_ref exp = make_ref(next, UNMARKED, UNFLAGGED);
            if (atomic_compare_exchange_weak(&prev->succ,
                                             &exp,
                                             make_ref(node, UNMARKED, UNFLAGGED))) {

                break;
            }

            struct list_node_ref result = prev->succ;
            if (!is_marked(result) && is_flagged(result)) {
                help_flagged(prev, result.node);
            }
            while (is_marked_atomic(prev->succ)) {
                prev = prev->backlink;
            }
        }

        search_from(key, prev, &prev, &next);

        if (prev->key == key) {
            mempool_free(&self->pool, node);
            errno = EEXIST;
            return -1;
        }
    }

    return 0;
}

/**
 *  try_flag desc.
 *
 *  @param  [in.out]    prev        prev desc.
 *  @param  [in]        target      target desc.
 *  @param  [out]       result_ptr  result_ptr desc.
 *  @return Returns true if succeed, false if failed.
 */
STATIC bool try_flag(struct list_node *prev, const struct list_node *target, struct list_node **result_ptr)
{
    *result_ptr = NULL;

    while (true) {
        if ((atomic_load(&prev->succ).node == target)
            && (!is_marked_atomic(prev->succ)
                && is_flagged_atomic(prev->succ))) {

            *result_ptr = prev;
            return false;
        }

        struct list_node_ref exp = get_unflagged_ref(target);
        if (atomic_compare_exchange_weak(&prev->succ,
                                         &exp,
                                         get_flagged_ref(target))) {

            *result_ptr = prev;
            return true;
        }

        struct list_node_ref result = prev->succ;
        if ((result.node == target) && !is_marked(result) && is_flagged(result)) {
            *result_ptr = prev;
            return false;
        }

        while (is_marked_atomic(prev->succ)) {
            prev = prev->backlink;
        }

        struct list_node *del;
        search_from(target->key - 1, prev, &prev, &del);

        if (del != target) {
            *result_ptr = NULL;
            errno = ENOENT;
            return false;
        }
    }
}

/**
 *  readers desc.
 */
static _Atomic(uint32_t) readers = 0;

/**
 *  @details    list_delete desc.
 *
 *  @param      [in,out]    lst lst desc.
 *  @param      [in]        key key desc.
 *  @param      [out]       val val desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int list_delete(list_t *lst, lkey_t key, void *val)
{
    SELFLIZE(struct list *, lst);

    struct list_node *prev, *del, *result;

    search_from(key - 1, self->head, &prev, &del);

    if (del->key != key) {
        errno = ENOENT;
        return -1;
    }

    bool deleted = try_flag(prev, del, &result);
    if (result != NULL) {
        help_flagged(result, del);
    }

    if (!deleted) {
        return -1;
    }

    if (val != NULL) {
        uint32_t orig;
        do {
            orig = atomic_load(&readers);
        } while ((orig == UINT32_MAX) || !atomic_compare_exchange_weak(&readers, &orig, orig + 1));
        memcpy(val, del->val, self->val_bytes);
        atomic_fetch_sub(&readers, 1);
    }
    mempool_free(&self->pool, del);

    return 0;
}

/**
 *  search desc.
 *
 *  @param  [in]    self    self desc.
 *  @param  [in]    key     key desc.
 *  @return Returns node object if found, NULL if not found.
 */
STATIC struct list_node *search(struct list *self, lkey_t key)
{
    struct list_node *curr, *_;

    search_from(key, self->head, &curr, &_);

    return (curr->key == key) ? curr : NULL;
}

/**
 *  @details    list_search desc.
 *
 *  @param      [in]    lst lst desc.
 *  @param      [in]    key key desc.
 *  @param      [out]   val val desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int list_search(list_t *lst, lkey_t key, void *val)
{
    SELFLIZE(struct list *, lst);

    struct list_node *target = search(self, key);
    if (target == NULL) {
        errno = ENOENT;
        return -1;
    }

    if (val != NULL) {
        uint32_t orig;
        do {
            orig = atomic_load(&readers);
        } while ((orig == UINT32_MAX) || !atomic_compare_exchange_weak(&readers, &orig, orig + 1));
        memcpy(val, target->val, self->val_bytes);
        atomic_fetch_sub(&readers, 1);
    }

    return 0;
}

/**
 *  @details    list_update desc.
 *
 *  @param      [in,out]    lst lst desc.
 *  @param      [in]        key key desc.
 *  @param      [in]        val val desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int list_update(list_t *lst, lkey_t key, const void *val)
{
    SELFLIZE(struct list *, lst);

    struct list_node *target = search(self, key);
    if (target == NULL) {
        errno = ENOENT;
        return -1;
    }

    if (val != NULL) {
        uint32_t orig;
        do {
            orig = atomic_load(&readers);
        } while ((orig != 0) || !atomic_compare_exchange_weak(&readers, &orig, UINT32_MAX));
        memcpy(target->val, val, self->val_bytes);
        atomic_init(&readers, 0);
    }

    return 0;
}

/**
 *  @details    list_dump desc.
 *
 *  @param      [in]    lst lst  desc.
 */
void list_dump(list_t *lst)
{
    SELFLIZE(struct list *, lst);

    printf("Lock-free List:\n");
    for (struct list_node *curr = atomic_load(&self->head->succ).node;
         curr != self->tail;
         curr = atomic_load(&curr->succ).node) {

        printf(" [%" PRIuPTR "]", curr->key);
    }
    printf("\n");
}
