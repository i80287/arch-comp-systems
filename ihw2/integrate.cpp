#include <iostream>
#include <cstdint>

template <uint32_t iters = 131072>
constexpr double IntegrateTrapezoid(int x0, int x1, double a, double b) noexcept {
    bool is_opposite = x0 > x1;
    if (is_opposite) {
        x0 ^= x1;
        x1 ^= x0;
        x0 ^= x1;
    }

    double ans = 0;
    double x = x0;
    double x_end = x1;
    double x_step = (x_end - x) / double(iters);
    if (x_step == 0) {
        return 0;
    }

    double prev_func_value = b * x * x * x + a;
    for (x += x_step; x <= x_end; x += x_step) {
        double func_value = b * x * x * x + a;
        ans += prev_func_value + func_value;
        prev_func_value = func_value;
    }

    ans = ans * x_step / 2;
    if (is_opposite) {
        ans = -ans;
    }

    return ans;
}

constexpr double IntegrateByFormula(int x0, int x1, double a, double b) noexcept {
    double x0_q = double(int64_t(x0) * int64_t(x0));
    x0_q *= x0_q;
    double x1_q = double(int64_t(x1) * int64_t(x1));
    x1_q *= x1_q;
    return a * double(int64_t(x1) - int64_t(x0)) + b * (x1_q - x0_q) / 4;
}

int main() {
    int x0 = -1;
    int x1 = 1;
    double a = 0.1;
    double b = 0.4;
    double I1 = IntegrateTrapezoid(x0, x1, a, b);
    double I2 = IntegrateByFormula(x0, x1, a, b);
    std::cout.precision(17);
    std::cout << std::boolalpha << std::showbase
        << "x_0 = " << x0 << "\nx1 = " << x1 << "\na = " << a << "\nb = " << b << '\n'
        << "trapezoid method I = " << I1 << '\n'
        << "math formula I = " << I2 << '\n'
        << "Correct: "<< (std::abs(I1 - I2) <= 0.0001) << '\n';
}

