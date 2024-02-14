#include <pthread.h>

#include <bit>
#include <chrono>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>

inline constexpr size_t kSmallArraySize = 100'000'000;
inline constexpr size_t kMiddleArraySize = 500'000'000;
inline constexpr size_t kMaxArraySize = 1000'000'000;

namespace chrono = std::chrono;

struct Timer {
    chrono::high_resolution_clock::time_point start;

    inline Timer() noexcept(noexcept(chrono::high_resolution_clock::time_point{
        chrono::high_resolution_clock::now()}))
        : start{chrono::high_resolution_clock::now()} {}

    inline ~Timer() {
        auto end = chrono::high_resolution_clock::now();
        int64_t milliseconds_elapsed =
            chrono::duration_cast<chrono::milliseconds>(end - start).count();
        printf("Elapsed milliseconds: %" PRIu64 "\n",
               uint64_t(milliseconds_elapsed));
    }
};

constexpr void FillArrayA(double array[], size_t array_size) noexcept {
    for (size_t i = 0; i != array_size; i++) {
        array[i] = double(i + 1);
    }
}

constexpr void FillArrayB(double array[], size_t array_size) noexcept {
    for (size_t i = 0; i != array_size; i++) {
        array[i] = double(array_size - i);
    }
}

struct Context {
    const double* array_a;
    const double* array_b;
    double* array_prod;
    size_t arr_chunk_size;
    size_t thread_num;
};

constexpr void* ArrayProduct(void* arg) noexcept {
    auto [array_a, array_b, array_prod, arr_chunk_size, thread_num] =
        *reinterpret_cast<Context*>(arg);
    size_t start_index = arr_chunk_size * thread_num;
    size_t end_index = start_index + arr_chunk_size;
    for (size_t i = start_index; i != end_index; i++) {
        array_prod[i] = array_a[i] * array_b[i];
    }

    return nullptr;
}

template <size_t kThreadsCount>
void ArrayProductMultiThread(const double array_a[], const double array_b[],
                             double array_prod[], size_t len) {
    static_assert(kThreadsCount != 0);
#ifdef PTHREAD_THREADS_MAX
    static_assert(kThreadsCount <= PTHREAD_THREADS_MAX);
#endif

    pthread_t threads[kThreadsCount - 1];
    Context contexts[kThreadsCount - 1];
    for (size_t i = 0; i != kThreadsCount - 1; i++) {
        contexts[i].array_a = array_a;
        contexts[i].array_b = array_b;
        contexts[i].array_prod = array_prod;
        contexts[i].arr_chunk_size = len / kThreadsCount;
        contexts[i].thread_num = i + 1;
        pthread_create(&threads[i], nullptr, &ArrayProduct,
                       reinterpret_cast<void*>(&contexts[i]));
    }

    Context main_ctx = {
        .array_a = array_a,
        .array_b = array_b,
        .array_prod = array_prod,
        .arr_chunk_size = len / kThreadsCount,
        .thread_num = 0,
    };
    ArrayProduct(reinterpret_cast<void*>(&main_ctx));

    for (pthread_t& thread : threads) {
        pthread_join(thread, nullptr);
    }
}

template <size_t kThreadsCount>
void MeasureArrayProduct(const double array_a[], const double array_b[],
                         double array_prod[], size_t len) {
    printf("Starting measurements using %zu threads, array size is %zu\n",
           kThreadsCount, len);
    Timer t;
    ArrayProductMultiThread<kThreadsCount>(array_a, array_b, array_prod, len);
}

int main() {
    for (size_t array_size : { kSmallArraySize, kMiddleArraySize, kMaxArraySize}) {    
        auto array_a = std::make_unique<double[]>(array_size);
        auto array_b = std::make_unique<double[]>(array_size);
        auto array_prod = std::make_unique<double[]>(array_size);
        FillArrayA(array_a.get(), array_size);
        FillArrayB(array_b.get(), array_size);

        MeasureArrayProduct<1>(array_a.get(), array_b.get(), array_prod.get(), array_size);
        MeasureArrayProduct<2>(array_a.get(), array_b.get(), array_prod.get(), array_size);
        MeasureArrayProduct<4>(array_a.get(), array_b.get(), array_prod.get(), array_size);
        MeasureArrayProduct<8>(array_a.get(), array_b.get(), array_prod.get(), array_size);
        MeasureArrayProduct<1000>(array_a.get(), array_b.get(), array_prod.get(), array_size);
    }
}
