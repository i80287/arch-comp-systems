#include <cstdio>
#include <cstdint>
#include <utility>

static constexpr bool SameSign(int32_t a, int32_t b) noexcept {
    return (a >= 0) ^ (b < 0);
}

static void Func(int32_t a, int32_t b, int32_t& q_ans, int32_t& r_ans) {
    if (b == 0) {
        q_ans = r_ans = 0;
        puts("Division by zero");
        return;
    }

    int32_t q = 0;
    int32_t r = 0;

    bool q_sign_carry = !SameSign(a, b);
    bool r_sign_carry = a < 0;
    if (a < 0) {
        a = -a;
    }

    if (b < 0) {
        b = -b;
    }

    while (a >= b) {
        a = a - b;
        q = q + 1;
    }

    r = a;

    q_ans = q_sign_carry ? -q : q;
    r_ans = r_sign_carry ? -r : r;
}

static void RunTests() {
    constexpr int32_t test_vars[][2] = {
        {   2,  4 },
        {   2, -4 },
        {  -2,  4 },
        {  -2, -4 },
        {  10,  4 },
        {  10, -4 },
        { -10,  4 },
        { -10, -4 },
        { -10, 12 },
        {   0,  0 },
        {   1,  0 },
        {  -1,  0 },
        {   0, 10 },
        {   0, -10 },
    };

    uint32_t test_number = 0;
    for (auto [a, b] : test_vars) {
        test_number++;

        int32_t func_q = 0;
        int32_t func_r = 0;
        Func(a, b, func_q, func_r);
        if (b == 0) {
            continue;
        }

        int32_t correct_q = a / b;
        int32_t correct_r = a % b;
        if (func_q != correct_q || func_r != correct_r) {
            fprintf(stderr,
                "Test %u failed: divmod(%d, %d) returned (r = %d, q = %d) instead of (r = %d, q= %d)\n",
                test_number,
                a,
                b,
                func_q,
                func_r,
                correct_q,
                correct_r);

            return;
        }
    }

    puts("All tests passed");
}

int main() {
    // int32_t a;
    // int32_t b;
    // fputs("Enter first i32 number\n> ", stdout);
    // if (scanf("%d", &a) != 1) {
    //     return 1;
    // }
    // fputs("Enter second i32 number\n> ", stdout);
    // if (scanf("%d", &b) != 1) {
    //     return 1;
    // }

    RunTests();

    // for (int32_t a = -4; a <= 4; a++) {
    //     for (int32_t b = -4; b <= 4; b++) {
    //         printf("SameSign(%d, %d) = %d\n", a, b, SameSign(a, b));
    //     }
    // }
}

