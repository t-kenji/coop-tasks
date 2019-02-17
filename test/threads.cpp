/** @file       threads.cpp
 *  @brief      Unit-test for Threads.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2019-02-17 create new.
 *  @copyright  Copyright (c) 2019 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#include <cstdio> // for debugging.
#include <cstdint>
#include <cerrno>
#include <atomic>
#include <catch2/catch.hpp>

using Catch::Matchers::Equals;

#include "utils.hpp"

#include "threads.h"
#include "future.h"

extern "C" {
#include "debug.h"
}

SCENARIO("スレッドが作成出来ること", tags("threads", "thrd_create", "thrd_current")) {

    GIVEN("特になし") {

        WHEN("スレッドを作成する") {
            auto runner = [&](void *arg) -> int {
                promise_t *prms = (promise_t *)arg;

                promise_set_value(prms, thrd_current());
                return 0;
            };

            THEN("作成出来ること") {
                thrd_t our_thr;
                promise_t prms = PROMISE_INITIALIZER;
                future_t *ftr = promise_get_future(&prms);

                REQUIRE(thrd_create(&our_thr, Lambda::ptr<int, void *>(runner), &prms) == 0);

                intmax_t their_thr;
                future_get_value(ftr, &their_thr);
                REQUIRE(thrd_equal(our_thr, (thrd_t)their_thr) != 0);

                thrd_join(our_thr, NULL);
            }
        }
    }
}

SCENARIO("スレッドがデタッチできること", tags("threads", "thrd_detach")) {

    GIVEN("スレッドが動作済みであること") {
        auto runner = [&](void *) -> int {
            return 0;
        };

        thrd_t thr;
        REQUIRE(thrd_create(&thr, Lambda::ptr<int, void *>(runner), NULL) == 0);

        WHEN("デタッチする") {

            THEN("デタッチ出来ること") {
                REQUIRE(thrd_detach(thr) == 0);

                // 副作用の確認.
                msleep(10);
                REQUIRE(thrd_join(thr, NULL) == -1);
                REQUIRE(errno == EINVAL);
            }
        }
    }
}

SCENARIO("スレッドの比較が出来ること", tags("threads", "thread_equal")) {

    GIVEN("2 スレッドが動作済みであること") {
        auto runner = [&](void *) -> int {
            return 0;
        };

        thrd_t our_thr, their_thr;
        REQUIRE(thrd_create(&our_thr, Lambda::ptr<int, void *>(runner), NULL) == 0);
        REQUIRE(thrd_create(&their_thr, Lambda::ptr<int, void *>(runner), NULL) == 0);

        WHEN("スレッドを比較する") {

            THEN("一致が判断できること") {
                REQUIRE(thrd_equal(our_thr, our_thr) != 0);
                REQUIRE(thrd_equal(their_thr, their_thr) != 0);
            }

            THEN("不一致が判断できること") {
                REQUIRE(thrd_equal(our_thr, their_thr) == 0);
            }
        }

        thrd_join(their_thr, NULL);
        thrd_join(our_thr, NULL);
    }
}

SCENARIO("スレッドが終了出来ること", tags("threads", "thrd_exit", "thrd_join")) {

    GIVEN("スレッドが動作済みであること") {
        std::atomic<bool> after_exit(false);
        auto runner = [&](void *) -> int {
            thrd_exit(10);
            after_exit = true;
            return 0;
        };

        thrd_t thr;
        REQUIRE(thrd_create(&thr, Lambda::ptr<int, void *>(runner), NULL) == 0);

        WHEN("スレッドで自身を終了させる") {

            THEN("終了出来ること") {
                int res;
                REQUIRE(thrd_join(thr, &res) == 0);
                REQUIRE(res == 10);
                REQUIRE(after_exit == false);
            }
        }
    }
}

SCENARIO("スレッドが停止出来ること", tags("threads", "thrd_suspend", "thrd_resume")) {

    GIVEN("スレッドが動作済みであること") {
        auto runner = [&](void *arg) -> int {
            promise_t *prms = (promise_t *)arg;
            int64_t base = getuptime(0);
            struct timespec ts = {
                .tv_sec = 1,
                .tv_nsec = 0,
            };
            thrd_sleep(&ts);
            promise_set_value(prms, getuptime(base));
            return 0;
        };

        WHEN("スレッドを停止させる") {
            promise_t prms = PROMISE_INITIALIZER;
            future_t *ftr = promise_get_future(&prms);

            thrd_t thr;
            REQUIRE(thrd_create(&thr, Lambda::ptr<int, void *>(runner), &prms) == 0);
            REQUIRE(thrd_detach(thr) == 0);

            THEN("停止出来ること") {
                REQUIRE(thrd_suspend(thr) == 0);
                msleep(1000);
                REQUIRE(thrd_resume(thr) == 0);
                int64_t elapsed = 0;
                future_get_value(ftr, &elapsed);
                REQUIRE(elapsed > 1900);
            }
        }
    }
}

SCENARIO("スレッドの名前を変更できること", tags("threads", "thrd_set_name", "thrd_get_name")) {

    GIVEN("スレッドが動作済みであること") {
        auto runner = [&](void *) -> int {
            struct timespec ts = {
                .tv_sec = 5,
                .tv_nsec = 0,
            };
            thrd_sleep(&ts);
            return 0;
        };

        thrd_t thr;
        REQUIRE(thrd_create(&thr, Lambda::ptr<int, void *>(runner), NULL) == 0);

        WHEN("スレッドの名前を設定する") {
            const char *name = "iwashi";
            INFO("名前: " + std::string(name));

            REQUIRE(thrd_set_name(thr, name) == 0);

            THEN("設定出来ること") {
                char buf[32] = {0};
                REQUIRE(thrd_get_name(thr, buf, sizeof(buf)) == 0);
                REQUIRE_THAT(buf, Equals(name));
            }
        }

        REQUIRE(thrd_cancel(thr) == 0);
        REQUIRE(thrd_join(thr, NULL) == 0);
    }
}
