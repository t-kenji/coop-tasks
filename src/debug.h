/** @file       debug.h
 *  @brief      Debuging and Testable.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2019-02-03 create new.
 *  @copyright  Copyright (c) 2018-2019 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#ifndef __TASKS_INTERNAL_DEBUG_H__
#define __TASKS_INTERNAL_DEBUG_H__

#if INTERNAL_TESTABLE == 1
#define STATIC
#define INLINE
#else
#define STATIC __attribute__((unused)) static
#define INLINE __attribute__((unused)) static inline
#endif

#define UNUSED_VARIABLE(x) (void)(x)

#if !defined(DEBUG_FILEOUT)
#define DEBUG_FILEOUT stdout
#endif

#if !defined(DEBUG_FILEERR)
#define DEBUG_FILEERR stderr
#endif

#if NODEBUG == 0
#define DEBUG(format, ...)                    \
    do {                                      \
        fprintf(DEBUG_FILEOUT,                \
                "%s:%d:%s " format "\n",      \
                __FILE__, __LINE__, __func__, \
                ##__VA_ARGS__);               \
    } while(0)
#else
#define DEBUG(format, ...)
#endif

#define ERROR(format, ...)                    \
    do {                                      \
        fprintf(DEBUG_FILEERR,                \
                "%s:%d:%s " format "\n",      \
                __FILE__, __LINE__, __func__, \
                ##__VA_ARGS__);               \
    } while(0)

#endif /* __TASKS_INTERNAL_DEBUG_H__ */
