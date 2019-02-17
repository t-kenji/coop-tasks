/** @file       utils.h
 *  @brief      Non-dependency useful functions.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2019-02-03 create new.
 *  @copyright  Copyright (c) 2018-2019 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#ifndef __TASKS_INTERNAL_UTILS_H__
#define __TASKS_INTERNAL_UTILS_H__

#define CAT_I(a, b) a ## b
#define CAT(a, b) CAT_I(a, b)

#define UNIQ(var) CAT(var, __LINE__)

#define generic_lock(obj)                           \
    _Generic((obj),                                 \
             pthread_mutex_t *: pthread_mutex_lock, \
             pthread_spinlock_t *: pthread_spin_lock)(obj)

#define generic_unlock(obj)                           \
    _Generic((obj),                                   \
             pthread_mutex_t *: pthread_mutex_unlock, \
             pthread_spinlock_t *: pthread_spin_unlock)(obj)

#define generic_cancellable_lock(obj)                                          \
    generic_lock(obj);                                                         \
    pthread_cleanup_push(                                                      \
        (void (*)(void *))_Generic((obj),                                      \
                                   pthread_mutex_t *: pthread_mutex_unlock,    \
                                   pthread_spinlock_t *: pthread_spin_unlock), \
        (obj))

#define generic_cancellable_unlock(obj) \
    generic_unlock(obj);                \
    pthread_cleanup_pop(0)

#define lock(obj)                             \
    void UNIQ(__locker__)(void (*fn)(void)) { \
        generic_cancellable_lock(obj);        \
        fn();                                 \
        generic_cancellable_unlock(obj);      \
    }                                         \
    auto void UNIQ(__lockee__)(void);         \
    UNIQ(__locker__)(UNIQ(__lockee__));       \
    void UNIQ(__lockee__)(void)

#define SELFLIZE(type, var) \
    type self = (type)(var)

#endif /* __TASKS_INTERNAL_UTILS_H__ */
