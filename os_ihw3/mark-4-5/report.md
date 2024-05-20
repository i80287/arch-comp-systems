# Отчёт о выполении ИДЗ №3

кормилицын владимир алексеевич, БПИ226

## Описание программ

**Для выполнения задачи были написаны 4 программы:**
* #### server.c - сервер, к которому подключаются клиенты (рабочие на участках) и который перенаправляет сообщения между ними, выводя информацию в консоль. Функции для инициализации стркутуры Server и работы с сетью вынесены в модуль server-tools.c.
* #### first-worker.c - клиент, эмулирующий работу работника на 1 участке.
* #### second-worker.c - клиент, эмулирующий работу работника на 2 участке.
* #### third-worker.c - клиент, эмулирующий работу работника на 3 участке.
#### Для работы с сетью все клиенты (рабочие) используют функции из модуля worker-tools.c.

Корретное завершение работы приложения возможно при нажатии сочетания клавиш Ctrl-C, в таком случае сервер пошлёт всем клиентам специальный сигнал о завершении работы и корректно деинициализирует все выделенные ресурсы.

---

### Сервер парсит входные аргументы, и, если порт указан верно, создаёт и инициализирует ресурсы, создаёт процесс, который будет опрашивать клиентом, а в основном процессе ждёт новых клиентов.

```c
static int run_server(uint16_t server_port) {
    if (!init_server(&server, server_port)) {
        return EXIT_FAILURE;
    }

    int ret = start_runtime_loop(&server);
    deinit_server(&server);
    printf("> Deinitialized server resources\n");
    return ret;
}

int main(int argc, const char* argv[]) {
    setup_signal_handler();

    ParseResultServer res = parse_args_server(argc, argv);
    if (res.status != PARSE_SUCCESS) {
        print_invalid_args_error_server(res.status, argv[0]);
        return EXIT_FAILURE;
    }

    return run_server(res.port);
}
```

```c
static int start_runtime_loop() {
    pthread_t poll_thread = 0;
    int ret = pthread_create(&poll_thread, NULL, &workers_poller, NULL);
    if (ret != 0) {
        errno = ret;
        perror("pthread_create");
        return EXIT_FAILURE;
    }

    printf("> Started polling thread\n> Server ready to accept connections\n");

    while (is_acceptor_running) {
        WorkerType type;
        size_t insert_index;
        server_accept_worker(&server, &type, &insert_index);
    }

    void* poll_ret       = NULL;
    int pthread_join_ret = pthread_join(poll_thread, &poll_ret);
    if (pthread_join_ret != 0) {
        errno = pthread_join_ret;
        perror("pthread_join");
    }

    send_shutdown_signal_to_all(&server);
    printf(
        "> Sent shutdown signals to all clients\n"
        "> Started waiting for %u seconds before closing the sockets...\n",
        (uint32_t)MAX_SLEEP_TIME);
    sleep(MAX_SLEEP_TIME);

    return (int)(uintptr_t)poll_ret;
}
```

### Процесс-поллер раз в некоторое время оправшивает клиентов, собирая булавки, которые они передают друг другу, в кольцевые буфферы `pins_1_to_2` и `pins_2_to_3`:
```c
static void* workers_poller(void* unused) {
    (void)unused;
    const struct timespec sleep_time = {
        .tv_sec  = 1,
        .tv_nsec = 500000000,
    };

    PinsQueue pins_1_to_2 = {0};
    PinsQueue pins_2_to_3 = {0};
    while (is_poller_running) {
        if (!poll_workers_on_the_first_stage(pins_1_to_2)) {
            fprintf(stderr, "> Could not poll workers on the first stage\n");
            break;
        }
        if (!poll_workers_on_the_second_stage(pins_1_to_2, pins_2_to_3)) {
            fprintf(stderr, "> Could not poll workers on the second stage\n");
            break;
        }
        if (!poll_workers_on_the_third_stage(pins_2_to_3)) {
            fprintf(stderr, "> Could not poll workers on the third stage\n");
            break;
        }
        if (nanosleep(&sleep_time, NULL) == -1) {
            if (errno != EINTR) {  // if not interrupted by the signal
                perror("nanosleep");
            }
            break;
        }
    }

    int32_t ret       = is_poller_running ? EXIT_FAILURE : EXIT_SUCCESS;
    is_poller_running = false;
    return (void*)(uintptr_t)(uint32_t)ret;
}
```

### Интерфейс модуля server-tools.h для работы с сетью (в отчёте без include'ов и у функций только сигнатуры):
```c
enum {
    MAX_NUMBER_OF_FIRST_WORKERS  = 3,
    MAX_NUMBER_OF_SECOND_WORKERS = 5,
    MAX_NUMBER_OF_THIRD_WORKERS  = 2,

    MAX_WORKERS_PER_SERVER = MAX_NUMBER_OF_FIRST_WORKERS +
                             MAX_NUMBER_OF_SECOND_WORKERS +
                             MAX_NUMBER_OF_THIRD_WORKERS,
    MAX_CONNECTIONS_PER_SERVER = MAX_WORKERS_PER_SERVER,
};

typedef struct WorkerMetainfo {
    char host[48];
    char port[16];
    char numeric_host[48];
    char numeric_port[16];
} WorkerMetainfo;

typedef struct Server {
    atomic_int sock_fd;
    struct sockaddr_in sock_addr;

    pthread_mutex_t first_workers_mutex;
    struct sockaddr_in first_workers_addrs[MAX_NUMBER_OF_FIRST_WORKERS];
    int first_workers_fds[MAX_NUMBER_OF_FIRST_WORKERS];
    struct WorkerMetainfo first_workers_info[MAX_NUMBER_OF_FIRST_WORKERS];
    volatile size_t first_workers_arr_size;

    pthread_mutex_t second_workers_mutex;
    struct sockaddr_in second_workers_addrs[MAX_NUMBER_OF_SECOND_WORKERS];
    int second_workers_fds[MAX_NUMBER_OF_SECOND_WORKERS];
    struct WorkerMetainfo second_workers_info[MAX_NUMBER_OF_SECOND_WORKERS];
    volatile size_t second_workers_arr_size;

    pthread_mutex_t third_workers_mutex;
    struct sockaddr_in third_workers_addrs[MAX_NUMBER_OF_THIRD_WORKERS];
    int third_workers_fds[MAX_NUMBER_OF_THIRD_WORKERS];
    struct WorkerMetainfo third_workers_info[MAX_NUMBER_OF_THIRD_WORKERS];
    volatile size_t third_workers_arr_size;
} Server[1];

bool init_server(Server server, uint16_t server_port);
void deinit_server(Server server);

bool server_accept_worker(Server server, WorkerType* type, size_t* insert_index);
void send_shutdown_signal_to_one(const Server server, WorkerType type, size_t index);
void send_shutdown_signal_to_first_workers(Server server);
void send_shutdown_signal_to_second_workers(Server server);
void send_shutdown_signal_to_third_workers(Server server);
void send_shutdown_signal_to_all_of_type(Server server, WorkerType type);
void send_shutdown_signal_to_all(Server server);

static inline bool server_lock_mutex(pthread_mutex_t* mutex);
static inline bool server_unlock_mutex(pthread_mutex_t* mutex);
static inline bool server_lock_first_mutex(Server server);
static inline bool server_unlock_first_mutex(Server server);
static inline bool server_lock_second_mutex(Server server);
static inline bool server_unlock_second_mutex(Server server);
static inline bool server_lock_third_mutex(Server server);
static inline bool server_unlock_third_mutex(Server server);
```

### Структура булавки и структура очереди булавок описаны в файле "pin.h":

```c
/// @brief Pin that workers pass to each other.
typedef struct Pin {
    int pin_id;
} Pin;

enum { PINS_QUEUE_MAX_SIZE = 16 };

typedef struct PinsQueue {
    Pin array[PINS_QUEUE_MAX_SIZE];
    size_t read_pos;
    size_t write_pos;
    size_t queue_size;
} PinsQueue[1];

static inline bool pins_queue_empty(const PinsQueue queue);
static inline bool pins_queue_full(const PinsQueue queue);
static inline bool pins_queue_try_put(PinsQueue queue, Pin pin);
static inline bool pins_queue_try_pop(PinsQueue queue, Pin* pin);
```

### Первый клиент (рабочий на 1 участке) парсит входные аргументы, и, если адрес порт указаны корректно, инициализирует ресурсы структуры Worker и запускает основной цикл:
```c
static int run_worker(const char* server_ip_address,
                      uint16_t server_port) {
    Worker worker;
    if (!init_worker(worker, server_ip_address, server_port,
                     FIRST_STAGE_WORKER)) {
        return EXIT_FAILURE;
    }

    print_worker_info(worker);
    int ret = start_runtime_loop(worker);
    deinit_worker(worker);
    return ret;
}

int main(int argc, const char* argv[]) {
    ParseResultWorker res = parse_args_worker(argc, argv);
    if (res.status != PARSE_SUCCESS) {
        print_invalid_args_error_worker(res.status, argv[0]);
        return EXIT_FAILURE;
    }

    return run_worker(res.ip_address, res.port);
}
```
```c

static int start_runtime_loop(Worker worker) {
    int ret = EXIT_SUCCESS;
    while (!worker_should_stop(worker)) {
        const Pin pin = receive_new_pin();
        log_received_pin(pin);
        bool is_ok = check_pin_crookness(pin);
        log_checked_pin(pin, is_ok);
        if (!is_ok) {
            continue;
        }

        if (worker_should_stop(worker)) {
            break;
        }
        if (!send_not_croocked_pin(worker, pin)) {
            ret = EXIT_FAILURE;
            break;
        }
        log_sent_pin(pin);
    }

    if (ret == EXIT_SUCCESS) {
        worker_handle_shutdown_signal();
    }

    printf("First worker is stopping...\n");
    return ret;
}
```

### Программы second-worker.c и third-worker.c устроены аналогично программе first-worker.c, но эмулируют работу рабочих на 2 и 3 участках соответственно.

### Интерфейс модуля client-tools.h для работы с сетью (в отчёте без include'ов и у функций только сигнатуры):
```c
typedef enum WorkerType {
    FIRST_STAGE_WORKER  = 1,
    SECOND_STAGE_WORKER = 2,
    THIRD_STAGE_WORKER  = 3,
} WorkerType;

static inline const char* worker_type_to_string(WorkerType type) {
    switch (type) {
        case FIRST_STAGE_WORKER:
            return "first stage worker";
        case SECOND_STAGE_WORKER:
            return "second stage worker";
        case THIRD_STAGE_WORKER:
            return "third stage worker";
        default:
            return "unknown stage worker";
    }
}

typedef struct Worker {
    int worker_sock_fd;
    WorkerType type;
    struct sockaddr_in server_sock_addr;
} Worker[1];

bool init_worker(Worker worker, const char* server_ip, uint16_t server_port, WorkerType type);
void deinit_worker(Worker worker);

bool worker_should_stop(const Worker worker);
void print_sock_addr_info(const struct sockaddr* address, socklen_t sock_addr_len);
static inline void print_worker_info(Worker worker);
static inline void worker_handle_shutdown_signal();
static inline void handle_errno(const char* cause);
static inline void handle_recvfrom_error(const char* bytes, ssize_t read_bytes);
static inline Pin receive_new_pin();
static inline bool check_pin_crookness(Pin pin);
static inline bool send_pin(int sock_fd, const struct sockaddr_in* sock_addr, Pin pin);
static inline bool send_not_croocked_pin(Worker worker, Pin pin);
static inline bool receive_pin(int sock_fd, Pin* rec_pin);
static inline bool receive_not_crooked_pin(Worker worker, Pin* rec_pin);
static inline void sharpen_pin(Pin pin);
static inline bool send_sharpened_pin(Worker worker, Pin pin);
static inline bool receive_sharpened_pin(Worker worker, Pin* rec_pin);
static inline bool check_sharpened_pin_quality(Pin sharpened_pin);
```

---
## Пример работы приложений

#### Запуск сервера без аргументов. Сервер сообщает о неверно введённых входных данных и выводит формат и пример использования.
![serv_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-4-5/images/img1.png?raw=true)

#### Запуск клиенты без аргументов. Клиент сообщает о неверно введённых входных данных и выводит формат и пример использования.
![clnt_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-4-5/images/img2.png?raw=true)

#### Запуск сервера. Сервер запускается на локальном адресе 127.0.0.1 (localhost) на порте 45235.
![img3](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-4-5/images/img3.png?raw=true)

#### Запуск клиента first-worker (рабочего на 1 площадке). Клиент подключается к серверу по адресу 127.0.0.1:45235 и выводит информацию о сервере (правая консоль). Сервер принимает клиента и выводит информацию о нём (левая консоль) (информация: "first stage worker"(address=localhost:36306 | 127.0.0.1:36306)). На момент создания скриншота клиент уже успел передать серверу одну проверенную на отсутствие кривизны булавку с id 1804289383. Сервер вывел информацию о том, что получил булавку с id 1804289383.
![img4](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-4-5/images/img4.png?raw=true)

#### Запуск клиента second-worker (рабочего на 2 площадке). Клиент подключается к серверу по адресу 127.0.0.1:45235 и выводит информацию о сервере (3-я консоль слева на право). Сервер принимает клиента и выводит информацию о нём (1-я консоль слева на право) (информация: "second stage worker"(address=localhost:59632 | 127.0.0.1:59632)). На момент создания скриншота сервер уже успел передать клиенту одну проверенную на отсутствие кривизны булавку с id 1804289383. Клиент вывел информацию о том, что получил булавку с id 1804289383 и начал затачивать её.
![img5](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-4-5/images/img5.png?raw=true)

#### Запуск клиента third-worker (рабочего на 3 площадке). Клиент подключается к серверу по адресу 127.0.0.1:45235 и выводит информацию о сервере (4-я консоль слева на право). Сервер принимает клиента и выводит информацию о нём (1-я консоль слева на право) (информация: "third stage worker"(address=localhost:58290 | 127.0.0.1:58290)).
![img6](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-4-5/images/img6.png?raw=true)

#### Работа приложения: 1-й клиент проверяет булавки, передаёт их серверу, он передаёт 2-ому клиенту, он оттачивает их, передаёт серверу, он передаёт их 3-ому клиенту, и он проверяет их, выводя результат проверки в консоль. Сервер логирует все события в системе.
![img7](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-4-5/images/img7.png?raw=true)

#### Завершение работы системы при нажатии сочетания клавиш Ctrl-C в консоли сервера. Сервер посылает сигнал о завершении клиентам, ждёт некоторое время, и после этого закрывает все сокеты и деинициализирует ресурсы.
![img8](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-4-5/images/img8.png?raw=true)
