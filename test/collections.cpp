/** @file       collections.cpp
 *  @brief      Unit-test for Collections.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2019-02-03 create new.
 *  @copyright  Copyright (c) 2019 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#include <catch2/catch.hpp>

using Catch::Matchers::Equals;

#include "utils.hpp"

#include "collections.h"
#include "threads.h"
#include "future.h"

extern "C" {
#include "debug.h"
}

SCENARIO("メモリプールからメモリを取得できること", tags("collections", "mempool", "mempool_alloc", "mempool_free")) {

    GIVEN("サイズの十分なメモリプールを作成する") {
        mpool_t mp;
        size_t capacity{100};

        INFO("容量: " + std::to_string(capacity));

        REQUIRE(mempool_create(&mp, sizeof(int), capacity) == 0);

        WHEN("メモリプールからメモリを取得する") {

            THEN("取得できること") {
                CHECK(mempool_alloc(&mp) != NULL);
                CHECK(mempool_alloc(&mp) != NULL);
                CHECK(mempool_alloc(&mp) != NULL);
            }
        }

        mempool_destroy(&mp);
    }

    GIVEN("メモリプールからメモリを取得しておく") {
        mpool_t mp;
        size_t capacity = 10;

        REQUIRE(mempool_create(&mp, sizeof(int), capacity) == 0);

        int *vals[] = {
            (int *)mempool_alloc(&mp),
            (int *)mempool_alloc(&mp),
            (int *)mempool_alloc(&mp),
            (int *)mempool_alloc(&mp),
            (int *)mempool_alloc(&mp),
        };
        CHECK(mempool_freeable(&mp) == capacity - lengthof(vals));
        CHECK(mempool_contains(&mp, vals[0]));
        CHECK(mempool_contains(&mp, vals[1]));
        CHECK(mempool_contains(&mp, vals[2]));
        CHECK(mempool_contains(&mp, vals[3]));
        CHECK(mempool_contains(&mp, vals[4]));

        WHEN("メモリプールにメモリを解放する") {
            for (auto &v : vals) {
                mempool_free(&mp, v);
            }

            THEN("解放できること") {
                CHECK(mempool_freeable(&mp) == capacity);
            }
        }

        WHEN("解放したメモリを再利用できること") {
            for (auto &v : vals) {
                mempool_free(&mp, v);
            }
            int *re_vals[] = {
                (int *)mempool_alloc(&mp),
                (int *)mempool_alloc(&mp),
                (int *)mempool_alloc(&mp),
                (int *)mempool_alloc(&mp),
                (int *)mempool_alloc(&mp),
                (int *)mempool_alloc(&mp),
                (int *)mempool_alloc(&mp),
                (int *)mempool_alloc(&mp),
                (int *)mempool_alloc(&mp),
                (int *)mempool_alloc(&mp),
            };

            THEN("再利用できること") {
                CHECK(mempool_contains(&mp, re_vals[0]));
                CHECK(mempool_contains(&mp, re_vals[1]));
                CHECK(mempool_contains(&mp, re_vals[2]));
                CHECK(mempool_contains(&mp, re_vals[3]));
                CHECK(mempool_contains(&mp, re_vals[4]));
                CHECK(mempool_contains(&mp, re_vals[5]));
                CHECK(mempool_contains(&mp, re_vals[6]));
                CHECK(mempool_contains(&mp, re_vals[7]));
                CHECK(mempool_contains(&mp, re_vals[8]));
                CHECK(mempool_contains(&mp, re_vals[9]));
                CHECK(mempool_freeable(&mp) == capacity - lengthof(re_vals));
            }
        }

        mempool_destroy(&mp);
    }
}

SCENARIO("キューを作成できること", tags("collections", "queue", "queue_create", "queue_destroy")) {

    GIVEN("特になし") {

        WHEN("キューを作成する") {
            que_t q;
            size_t capacity{1};

            INFO("容量: " + std::to_string(capacity));

            THEN("キューが作成できること") {
                REQUIRE(queue_create(&q, sizeof(int), capacity) == 0);
                queue_destroy(&q);
            }
        }

        WHEN("キューを作成する") {
            que_t q;
            size_t capacity{10000};

            INFO("容量: " + std::to_string(capacity));

            THEN("キューが作成できること") {
                REQUIRE(queue_create(&q, sizeof(int), capacity) == 0);
                queue_destroy(&q);
            }
        }
    }
}

SCENARIO("キューにデータを追加できること", tags("collections", "queue", "queue_enqueue")) {

    GIVEN("サイズの十分なキューを作成する") {
        que_t q;
        size_t capacity{10};

        INFO("容量: " + std::to_string(capacity));

        REQUIRE(queue_create(&q, sizeof(int), capacity) == 0);

        WHEN("キューにデータを追加する") {
            int data{10};

            INFO("データ: " + std::to_string(data));

            THEN("データが追加できること") {
                CHECK(queue_enqueue(&q, &data) == 0);
            }
        }

        queue_destroy(&q);
    }
}

SCENARIO("キューからデータを取得できること", tags("collections", "queue", "queue_dequeue")) {

    GIVEN("キューにデータを追加しておく") {
        que_t q;
        size_t capacity{10};
        int data{10};

        INFO("容量: " + std::to_string(capacity));
        INFO("データ: " + std::to_string(data));

        REQUIRE(queue_create(&q, sizeof(data), capacity) == 0);
        CHECK(queue_enqueue(&q, &data) == 0);

        WHEN("キューからデータを取得する") {

            THEN("データが取得できること") {
                int result{-1};
                CHECK(queue_dequeue(&q, &result) == 0);
                CHECK(result == data);
            }
        }

        queue_destroy(&q);
    }

    GIVEN("キューにデータを追加しておく") {
        que_t q;
        size_t capacity{10};
        int data[]{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        INFO("容量: " + std::to_string(capacity));
        INFO("データ: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]");

        REQUIRE(queue_create(&q, sizeof(data[0]), capacity) == 0);
        for (auto &d : data) {
            queue_enqueue(&q, &d);
        }

        WHEN("キューからデータを取得する") {

            THEN("データが取得できること") {
                int result{-1};
                CHECK(queue_dequeue(&q, &result) == 0);
                CHECK(result == data[0]);
                CHECK(queue_dequeue(&q, &result) == 0);
                CHECK(result == data[1]);
                CHECK(queue_dequeue(&q, &result) == 0);
                CHECK(result == data[2]);
                CHECK(queue_dequeue(&q, &result) == 0);
                CHECK(result == data[3]);
                CHECK(queue_dequeue(&q, &result) == 0);
                CHECK(result == data[4]);
                CHECK(queue_dequeue(&q, &result) == 0);
                CHECK(result == data[5]);
                CHECK(queue_dequeue(&q, &result) == 0);
                CHECK(result == data[6]);
                CHECK(queue_dequeue(&q, &result) == 0);
                CHECK(result == data[7]);
                CHECK(queue_dequeue(&q, &result) == 0);
                CHECK(result == data[8]);
                CHECK(queue_dequeue(&q, &result) == 0);
                CHECK(result == data[9]);
            }
        }

        queue_destroy(&q);
    }

}

SCENARIO("リストを作成できること", tags("collections", "list", "list_create", "list_destroy")) {

    GIVEN("特になし") {

        WHEN("リストを作成する") {
            list_t lst;
            size_t capacity{1};

            INFO("容量: " + std::to_string(capacity));

            THEN("リストが作成できること") {
                REQUIRE(list_create(&lst, sizeof(int), capacity) == 0);
                list_destroy(&lst);
            }
        }

        WHEN("リストを作成する") {
            list_t lst;
            size_t capacity{10000};

            INFO("容量: " + std::to_string(capacity));

            THEN("リストが作成できること") {
                REQUIRE(list_create(&lst, sizeof(int), capacity) == 0);
                list_destroy(&lst);
            }
        }
    }
}

SCENARIO("リストにデータを追加できること", tags("collections", "list", "list_insert")) {

    GIVEN("サイズの十分なリストを作成する") {
        list_t lst;
        size_t capacity{10};

        INFO("容量: " + std::to_string(capacity));

        REQUIRE(list_create(&lst, sizeof(int), capacity) == 0);

        WHEN("リストにデータを追加する") {
            int data{10};
            lkey_t key{1};

            INFO("キー: " + std::to_string(key));
            INFO("データ: " + std::to_string(data));

            THEN("データが追加できること") {
                CHECK(list_insert(&lst, key, &data) == 0);
            }
        }

        WHEN("同一のキーにデータを追加する") {
            int data{10};
            lkey_t key{1};

            INFO("キー: " + std::to_string(key));
            INFO("データ: " + std::to_string(data));

            CHECK(list_insert(&lst, key, &data) == 0);

            THEN("データが追加できないこと") {
                CHECK(list_insert(&lst, key, &data) == -1);
                CHECK(errno == EEXIST);
            }
        }

        list_destroy(&lst);
    }

    GIVEN("サイズの十分ではないリストを作成する") {
        list_t lst;
        size_t capacity{1};

        INFO("容量: " + std::to_string(capacity));

        REQUIRE(list_create(&lst, sizeof(int), capacity) == 0);

        WHEN("リストに容量を超えるデータを追加する") {
            int data{10};
            lkey_t key{1};

            CHECK(list_insert(&lst, key, &data) == 0);

            THEN("データが追加できないこと") {
                CHECK(list_insert(&lst, key + 1, &data) == -1);
                CHECK(errno == ENOMEM);
            }
        }

        list_destroy(&lst);
    }

}

SCENARIO("リストからデータを削除 (取得) できること", tags("collections", "list", "list_delete")) {

    GIVEN("リストにデータを追加しておく") {
        list_t lst;
        size_t capacity{10};
        int data{10};
        lkey_t key{1};

        INFO("容量: " + std::to_string(capacity));
        INFO("キー: " + std::to_string(key));
        INFO("データ: " + std::to_string(data));

        REQUIRE(list_create(&lst, sizeof(data), capacity) == 0);
        CHECK(list_insert(&lst, key, &data) == 0);

        WHEN("リストからデータを削除 (取得) する") {

            THEN("データが削除 (取得) できること") {
                int result{-1};
                CHECK(list_delete(&lst, key, &result) == 0);
                CHECK(result == data);
            }
        }

        WHEN("存在しないデータを削除する") {

            THEN("データが削除できないこと") {
                int result{-1};
                CHECK(list_delete(&lst, key + 1, &result) == -1);
                CHECK(errno == ENOENT);
            }
        }

        list_destroy(&lst);
    }
}

SCENARIO("リストのデータを取得 / 更新できること", tags("collections", "list", "list_search", "list_update")) {

    GIVEN("リストにデータを追加しておく") {
        list_t lst;
        size_t capacity{10};
        int data{10};
        lkey_t key{1};

        INFO("データ: " + std::to_string(data));

        REQUIRE(list_create(&lst, sizeof(data), capacity) == 0);
        CHECK(list_insert(&lst, key, &data) == 0);

        WHEN("リストからデータを取得する") {

            THEN("データが取得できること") {
                int result{-1};
                CHECK(list_search(&lst, key, &result) == 0);
                CHECK(result == data);
            }
        }

        WHEN("リストのデータを更新する") {
            int updata{11};

            INFO("更新データ: " + std::to_string(updata));

            CHECK(list_update(&lst, key, &updata) == 0);

            THEN("データが更新できること") {
                int result{-1};
                CHECK(list_search(&lst, key, &result) == 0);
                CHECK(result == updata);
            }
        }

        list_destroy(&lst);
    }

    GIVEN("リストにデータを追加しておく") {
        list_t lst;
        size_t capacity{10};
        char data[32]{"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"};
        lkey_t key{1};

        INFO("データ: " + std::string(data));

        REQUIRE(list_create(&lst, sizeof(data), capacity) == 0);
        CHECK(list_insert(&lst, key, &data) == 0);

        WHEN("リストのデータを並列に更新する") {
            auto runner = [&](void *arg) -> int {
                char *updata = (char *)arg;
                while (true) {
                    pthread_testcancel();
                    list_update(&lst, key, updata);
                }
                return 0;
            };

            char updata_a[32]{"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"};
            char updata_b[32]{"ccccccccccccccccccccccccccccccc"};
            char updata_c[32]{"ddddddddddddddddddddddddddddddd"};
            thrd_t thr_a, thr_b, thr_c;
            CHECK(thrd_create(&thr_a, Lambda::ptr<int, void *>(runner), updata_a) == 0);
            CHECK(thrd_create(&thr_b, Lambda::ptr<int, void *>(runner), updata_b) == 0);
            CHECK(thrd_create(&thr_c, Lambda::ptr<int, void *>(runner), updata_c) == 0);

            THEN("データが破損しないこと") {
                auto verifier = [](const char *d) {
                    char c = d[0];
                    for (size_t i = 1; i < 32 - 1; ++i) {
                        if (d[i] != c) {
                            return false;
                        }
                    }
                    return true;
                };

                char result[32]{""};
                for (int i = 0; i < 1000000; ++i) {
                    list_search(&lst, key, &result);

                    if (!verifier(result)) {
                        break;
                    }
                }
                REQUIRE_THAT(std::string(result), Equals(updata_a) || Equals(updata_b) || Equals(updata_c));
            }

            thrd_cancel(thr_c);
            thrd_cancel(thr_b);
            thrd_cancel(thr_a);
            thrd_join(thr_c, NULL);
            thrd_join(thr_b, NULL);
            thrd_join(thr_a, NULL);
        }

        list_destroy(&lst);
    }

}
