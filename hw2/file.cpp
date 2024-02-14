#include <cstdio>
#include <cmath>

int f(int a0) {
    int a5 = a0 >> 31;
    return a0 = (a0 ^ a5) - a5;
}

// return carry ? -a0 : a0;
int g(int a0, bool carry) {
    int c = static_cast<int>(static_cast<unsigned>(carry) << 31) >> 31;
    return (a0 ^ c) - c;
}

int main() {
    // for (int a = -24; a <= 1; a++) {
    //     printf("%d ", f(a));
    // }
    // putchar('\n');

    // printf("|2147483647| = %d\n", f(2147483647));
    // printf("|-2147483647| = %d\n", f(-2147483647));
    // printf("|-2147483648| = %d\n", f(static_cast<int>(1u << 31)));

    // printf("10 = %d\n", g(10, false));
    // printf("-10 = %d\n", g(10, true));
    // printf("10 = %d\n", g(-10, true));
    // printf("-10 = %d\n", g(-10, false));
    // printf("0 = %d\n", g(0, true));
    // printf("0 = %d\n", g(0, false));

    // for (unsigned a = 2; a <= 100; a++) {
    //     for (unsigned b = 2; b <= 100; b++) {
    //         unsigned n = (a * a + b * b) / (a * b + 1);
    //         if ((a * a + b * b) % (a * b + 1) == 0) {
    //             printf("%u %u %u; D = %u\n", a, b, n, b * b * n * n - 4 * b * b + 4 * n);
    //             a = 101;
    //             break;
    //         }
    //     }
    // }

    return 0;
}
