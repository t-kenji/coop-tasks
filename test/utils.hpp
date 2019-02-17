/** @file   utils.hpp
 *  @brief  Unit-test utilities.
 *
 *  @author t-kenji <protect.2501@gmail.com>
 *  @date   2019-02-03 create new.
 */
#ifndef __TASKS_TEST_UTILS_H__
#define __TASKS_TEST_UTILS_H__

#define lengthof(array) (sizeof(array)/sizeof(array[0]))

template<typename First, typename ...Rest>
constexpr std::string tags(const First first, const Rest ...rest)
{
    const First args[] = {first, rest...};
    std::string tag_str = "";
    for (size_t i = 0; i < sizeof(args)/sizeof(args[0]); ++i) {
        tag_str += "[" + std::string(args[i]) + "]";
    }
    return tag_str;
}

/**
 *  @sa https://stackoverflow.com/a/33047781
 */
struct Lambda {
    template<typename Tret, typename Targ, typename T>
    static Tret lambda_ptr_exec(Targ arg) {
        return (Tret) (*(T *)fn<T>())(arg);
    }

    template<typename Tret = void, typename Targ = void *, typename Tfp = Tret(*)(Targ), typename T>
    static Tfp ptr(T& t) {
        fn<T>(&t);
        return (Tfp) lambda_ptr_exec<Tret, Targ, T>;
    }

    template<typename T>
    static void *fn(void *new_fn = nullptr) {
        static void *fn;
        if (new_fn != nullptr) {
            fn = new_fn;
        }
        return fn;
    }
};

int msleep(long msec);
int64_t getuptime(int64_t base);

#endif // __TASKS_TEST_UTILS_H__
