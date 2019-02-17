/** @file   main.cpp
 *  @brief  Catch2 テストフレームワーク.
 *
 *  @author takahashi@co-nss.co.jp
 *  @date   2019-01-07 新規作成.
 */
#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"

/**
 *  スタートアップ.
 *
 *  @param  [in]    argc    コマンドライン引数の数.
 *  @param  [in]    argv    コマンドライン引数の文字列配列.
 *  @return 正常終了時は 0 が返り, 失敗時はそれ以外が返る.
 */
int main(int argc, char **argv)
{
    return Catch::Session().run(argc, argv);
}
