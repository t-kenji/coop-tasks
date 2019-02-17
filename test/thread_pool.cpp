/** @file       thread_pool.cpp
 *  @brief      Unit-test for Thread-pool.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2019-01-08 create new.
 *  @copyright  Copyright (c) 2019 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#include <cstdint>
#include <catch2/catch.hpp>

using Catch::Matchers::Equals;

#include "utils.hpp"

#include "threads.h"
#include "future.h"
#include "thread_pool.h"

extern "C" {
#include "debug.h"
}

SCENARIO("スレッドプールが作成できること", tags("thread_pool", "thrdpool_create")) {

    GIVEN("特になし") {

        WHEN("スレッドプールを作成する") {
            size_t num_workers = 10;

            INFO("ワーカー数: " + std::to_string(num_workers));

            THEN("作成できること") {
                tpool_t tp;

                tp = thrdpool_create(num_workers);
                REQUIRE(tp != NULL);

                thrdpool_destroy(tp);
            }
        }
    }
}

SCENARIO("ジョブが実行できること", tags("thread_pool", "thrdpool_job_init")) {

    GIVEN("特になし") {

        WHEN("ジョブを作成する") {

            THEN("作成できること") {
                auto runner = [&](void *) -> int {
                    return 0;
                };

                job_t job;
                REQUIRE(thrdpool_job_init(&job, Lambda::ptr<int, void *>(runner), (void *)999) == 0);
                REQUIRE(job.func == Lambda::ptr<int, void *>(runner));
                REQUIRE(job.arg == (void *)(intptr_t)999);
            }
        }
    }

    GIVEN("スレッドプールを作成しておく") {
        size_t num_workers = 10;
        tpool_t tp;

        tp = thrdpool_create(num_workers);
        REQUIRE(tp != NULL);

        WHEN("ジョブを追加する") {
            intmax_t our_val = 999;

            promise_t prms = PROMISE_INITIALIZER;
            future_t *ftr = promise_get_future(&prms);
            auto runner = [&](void *arg) -> int {
                promise_set_value(&prms, (intmax_t)arg);
                return 0;
            };

            job_t job;
            CHECK(thrdpool_job_init(&job, Lambda::ptr<int, void *>(runner), (void *)our_val) == 0);
            CHECK(thrdpool_add(tp, &job) == 0);

            THEN("ジョブが実行されること") {
                intmax_t their_val;
                future_get_value(ftr, &their_val);
                CHECK(our_val == their_val);
            }
        }

        WHEN("ジョブからジョブを追加する") {
            intmax_t our_val = 999;

            promise_t prms = PROMISE_INITIALIZER;
            future_t *ftr = promise_get_future(&prms);
            auto runner = [&](void *arg) -> int {
                promise_set_value(&prms, (intmax_t)arg);
                return 0;
            };
            auto runner0 = [&](void *arg) -> int {
                job_t job0;
                thrdpool_job_init(&job0, Lambda::ptr<int, void *>(runner), arg);
                thrdpool_add(tp, &job0);
                return 0;
            };

            job_t job;
            CHECK(thrdpool_job_init(&job, Lambda::ptr<int, void *>(runner0), (void *)our_val) == 0);
            CHECK(thrdpool_add(tp, &job) == 0);

            THEN("ジョブが実行されること") {
                intmax_t their_val;
                future_get_value(ftr, &their_val);
                CHECK(our_val == their_val);
            }
        }

        thrdpool_destroy(tp);
    }
}
