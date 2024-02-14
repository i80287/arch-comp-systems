#include <pthread.h>
#include <semaphore.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <tuple>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#include <unistd.h>
#endif

inline constexpr uint32_t kNumGenThreadsCount = 20;
inline constexpr uint32_t kAddingThreadsCount = kNumGenThreadsCount - 1;

static class CyclicQueue {
public:
    /// @brief Ctor initializes lock primitives needed for
    ///        the correct work across different thread.
    ///        If any initialization fails, std::abort() is called.
    CyclicQueue() noexcept;

    CyclicQueue(const CyclicQueue&) = delete;
    CyclicQueue(CyclicQueue&&) = delete;

    CyclicQueue& operator=(const CyclicQueue&) = delete;
    CyclicQueue& operator=(CyclicQueue&&) = delete;

    ~CyclicQueue();

    /// @brief This method will push num to the queue.
    ///        This method is considered to be thread safe.
    /// @param num 
    void Push(uint32_t num) noexcept;

    /// @brief This method will wait until two numbers are available 
    ///        and return them as soon as they become available.
    ///        This method is not thread safe and should not
    ///        be called from different threads for one queue!!!
    /// @return 
    std::pair<uint32_t, uint32_t> PopPair() noexcept;

    uint32_t Front() const noexcept;

    uint32_t Back() const noexcept;
private:
    /// @brief Function called in the CyclicQueue().
    ///        Prints errormessage to the stderr and then calls std::abort().
    /// @param cause 
    [[noreturn]] static void OnInitError(const char* cause) noexcept;

    static constexpr size_t kMaxSize = 64;
    // Make it power of 2 for the fast mod operation
    static_assert(kMaxSize != 0 && (kMaxSize & (kMaxSize - 1)) == 0);

    uint32_t queue_data_[kMaxSize] = {};
    size_t read_index_ = 0;
    size_t write_index_ = 0;

    pthread_mutex_t queue_lock_;
    // Counter of the available numbers in the queue.
    sem_t read_semaphore_;
} queue;

void CyclicQueue::OnInitError(const char* cause) noexcept {
    int errno_code = errno;
    std::fprintf(stderr,
                 "Fatal error: could not init %s. Errno: %d. Error msg: %s\n",
                 cause, errno_code, std::strerror(errno_code));
    std::abort();
}

CyclicQueue::CyclicQueue() noexcept {
    if (pthread_mutex_init(&queue_lock_, nullptr) != 0) {
        OnInitError("queue lock");
    }

    /**
     * Default initialization: 0 numbers can read, initial value = 0
     */
    if (sem_init(&read_semaphore_, 0, 0) != 0) {
        pthread_mutex_destroy(&queue_lock_);
        OnInitError("read semaphore");
    }
}

CyclicQueue::~CyclicQueue() {
    sem_destroy(&read_semaphore_);
    pthread_mutex_destroy(&queue_lock_);
}

void CyclicQueue::Push(uint32_t num) noexcept {
    pthread_mutex_lock(&queue_lock_);
    queue_data_[write_index_] = num;
    write_index_ = (write_index_ + 1) % kMaxSize;
    pthread_mutex_unlock(&queue_lock_);
    // New number available for reading, increase counter.
    sem_post(&read_semaphore_);
}

std::pair<uint32_t, uint32_t> CyclicQueue::PopPair() noexcept {
    /**
     * Wait until two numbers will be available for reading.
     * Decrease counter by 2.
     */
    sem_wait(&read_semaphore_);
    sem_wait(&read_semaphore_);
    uint32_t first = queue_data_[read_index_];
    read_index_ = (read_index_ + 1) % kMaxSize;
    uint32_t second = queue_data_[read_index_];
    read_index_ = (read_index_ + 1) % kMaxSize;
    return { first, second };
}

uint32_t CyclicQueue::Front() const noexcept {
    return queue_data_[read_index_];
}

uint32_t CyclicQueue::Back() const noexcept {
    return queue_data_[(write_index_ + kMaxSize - 1) % kMaxSize];
}

template <uint32_t kFromSeconds, uint32_t kToSeconds>
static void RandSleep() noexcept {
    static_assert(kFromSeconds <= kToSeconds);
    uint32_t sleep_time_sec = uint32_t(rand()) % (kToSeconds - kFromSeconds + 1) + kFromSeconds;
#ifdef _WIN32
    Sleep(static_cast<DWORD>(sleep_time_sec * 1000));
#else
    sleep(sleep_time_sec);
#endif
}

static void* NumberGenerator(void* arg) noexcept {
    uint32_t thread_num = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(arg));

    printf("[Number generator thread %u] [Started generating number]\n", thread_num);
    fflush(stdout);
    RandSleep<1, 7>();
    uint32_t num = thread_num;

    printf("[Number generator thread %u] [Pushing number %u to the buffer]\n", thread_num, num);
    fflush(stdout);
    queue.Push(num);

    return nullptr;
}

static void* Adder([[maybe_unused]] void* arg) noexcept {
    auto [thread_num, num1, num2] =
        *reinterpret_cast<std::tuple<uint32_t, uint32_t, uint32_t>*>(arg);

    printf("[Adding thread %u] [Started summing up numbers %u and %u]\n", thread_num, num1, num2);
    fflush(stdout);

    RandSleep<3, 6>();
    uint32_t res = num1 + num2;

    printf("[Adding thread %u] [Pushing number %u to the buffer]\n", thread_num, res);
    fflush(stdout);

    queue.Push(res);
    return nullptr;
}

template <size_t kThreadsCount>
static void StartNumGenThreads(pthread_t (&num_gen_threads)[kThreadsCount]) noexcept {
    for (uint32_t i = 0; i < kThreadsCount; i++) {
        uint32_t thread_num = i + 1;
        printf("[Starting number_generator_thread %u]\n", thread_num);

        static_assert(sizeof(void*) == sizeof(uintptr_t));
        void* arg = reinterpret_cast<void*>(static_cast<uintptr_t>(thread_num));
        pthread_create(&num_gen_threads[i], nullptr, &NumberGenerator, arg);
    }
}

template <size_t kThreadsCount>
static void JoinThreads(const pthread_t (&num_gen_threads)[kThreadsCount]) noexcept {
    for (pthread_t num_get_thread : num_gen_threads) {
        pthread_join(num_get_thread, nullptr);
    }
}

static void AccumulateResults() noexcept {
    pthread_t adding_threads[kAddingThreadsCount];
    std::tuple<uint32_t, uint32_t, uint32_t> args[kAddingThreadsCount];

    for (uint32_t iter = 0; iter < kAddingThreadsCount; iter++) {
        auto [num1, num2] = queue.PopPair();
        uint32_t thread_num = iter + 1;
        printf("[Starting adding_thread %u] [Numbers: %u, %u]\n",
               thread_num, num1, num2);
        fflush(stdout);

        args[iter] = { thread_num, num1, num2 };
        pthread_create(&adding_threads[iter], nullptr, &Adder,
                       reinterpret_cast<void*>(&args[iter]));
    }

    JoinThreads(adding_threads);
    printf("[Stopped adding numbers] [Result: %u]\n", queue.Front());
    fflush(stdout);
}

int main() {
    pthread_t num_gen_threads[kNumGenThreadsCount];
    StartNumGenThreads(num_gen_threads);
    AccumulateResults();
    JoinThreads(num_gen_threads);
}
