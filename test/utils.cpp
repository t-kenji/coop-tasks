/** @file   utils.cpp
 *  @brief  Unit-test utilities.
 *
 *  @author t-kenji <protect.2501@gmail.com>
 *  @date   2019-02-03 create new.
 */
#include <string>
#include <cerrno>
#include <ctime>

#include "utils.hpp"

int msleep(long msec)
{
    struct timespec req, rem = {msec / 1000, (msec % 1000) * 1000000};
    int ret;

    do {
        req = rem;
        ret = clock_nanosleep(CLOCK_MONOTONIC, 0, &req, &rem);
    } while ((ret != 0) && (errno == EINTR));

    return ret;
}

int64_t getuptime(int64_t base)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return -1;
    }
    return (ts.tv_sec * 1000 + (ts.tv_nsec / 1000000)) - base;
}
