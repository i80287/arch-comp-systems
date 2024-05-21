# Отчёт о выполении ИДЗ №3

кормилицын владимир алексеевич, БПИ226

## Описание программ

**Для выполнения задачи были написаны 5 программы:**
* #### server.c - сервер, к которому подключаются клиенты (рабочие на участках) и который перенаправляет сообщения между ними, выводя информацию в консоль. Функции для инициализации стркутуры Server и работы с сетью вынесены в модуль server-tools.c.
* #### first-worker.c - клиент, эмулирующий работу работника на 1 участке.
* #### second-worker.c - клиент, эмулирующий работу работника на 2 участке.
* #### third-worker.c - клиент, эмулирующий работу работника на 3 участке.
* #### logs-collector.c - клиент, подключающийся к серверу и получающий логи от него.
#### Для работы с сетью все клиенты (рабочие) используют функции из модуля client-tools.c.

Корретное завершение работы приложения возможно при нажатии сочетания клавиш Ctrl-C, в таком случае сервер пошлёт всем клиентам специальный сигнал о завершении работы и корректно деинициализирует все выделенные ресурсы. Если клиент разрывает соединение, сервер корректно обрабатывает это и удаляет клиента из внтуреннего массива клиентов.

---

### Сервер парсит входные аргументы, и, если порт указан верно, создаёт и инициализирует ресурсы, создаёт процесс, который будет опрашивать клиентом, и процесс, рассылающий логи, а в основном процессе ждёт новых клиентов.

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
    pthread_t poll_thread;
    if (!create_polling_thread(&poll_thread)) {
        return EXIT_FAILURE;
    }
    printf("> Started polling thread\n");

    pthread_t logs_thread;
    if (!create_logging_thread(&logs_thread)) {
        pthread_detach(poll_thread);
        return EXIT_FAILURE;
    }
    printf("> Started logging thread\n");

    ServerLog log = {0};
    printf("> Server ready to accept connections\n");
    while (is_acceptor_running) {
        ClientType type;
        size_t insert_index;
        server_accept_client(&server, &type, &insert_index);
        if (!nonblocking_enqueue_log(&server, &log)) {
            fprintf(stderr, "> Logs queue if full. Can't add new log to the queue\n");
        }
    }

    int ret1 = join_thread(logs_thread);
    int ret2 = join_thread(poll_thread);

    printf("Started sending shutdown signals to all clients\n");
    send_shutdown_signal_to_all(&server);
    printf(
        "> Sent shutdown signals to all clients\n"
        "> Started waiting for %u seconds before closing the sockets\n",
        (uint32_t)MAX_SLEEP_TIME);
    sleep(MAX_SLEEP_TIME);

    return ret1 | ret2;
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
        if (!nonblocking_poll_workers_on_the_first_stage(&server, pins_1_to_2)) {
            fprintf(stderr, "> Could not poll workers on the first stage\n");
            break;
        }
        if (!nonblocking_poll_workers_on_the_second_stage(&server, pins_1_to_2, pins_2_to_3)) {
            fprintf(stderr, "> Could not poll workers on the second stage\n");
            break;
        }
        if (!nonblocking_poll_workers_on_the_third_stage(&server, pins_2_to_3)) {
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

### Процесс-логгер ожидает логи из очереди логов (паттерн очереди, поддерживающей работу с несколькими процессами - producer-consumer с 1 мьютексом и 2 семафорами) и отправляет их всем подключённым клиентам-логгерам:
```c
static void* logs_sender(void* unused) {
    (void)unused;
    const struct timespec sleep_time = {
        .tv_sec  = 1,
        .tv_nsec = 500000000,
    };

    ServerLog log = {0};
    while (is_logger_running) {
        if (!dequeue_log(&server, &log)) {
            fprintf(stderr, "> Could not get next log\n");
            break;
        }

        send_server_log(&server, &log);
        if (nanosleep(&sleep_time, NULL) == -1) {
            if (errno != EINTR) {  // if not interrupted by the signal
                perror("nanosleep");
            }
            break;
        }
    }

    int32_t ret       = is_logger_running ? EXIT_FAILURE : EXIT_SUCCESS;
    is_poller_running = false;
    return (void*)(uintptr_t)(uint32_t)ret;
}
```

### Интерфейс модуля server-tools.h для работы с сетью (в отчёте без include'ов и у функций только сигнатуры):
```c
enum {
    MAX_NUMBER_OF_FIRST_WORKERS   = 3,
    MAX_NUMBER_OF_SECOND_WORKERS  = 5,
    MAX_NUMBER_OF_THIRD_WORKERS   = 2,
    MAX_NUMBER_OF_LOGS_COLLECTORS = 6,

    MAX_WORKERS_PER_SERVER =
        MAX_NUMBER_OF_FIRST_WORKERS + MAX_NUMBER_OF_SECOND_WORKERS + MAX_NUMBER_OF_THIRD_WORKERS,
    MAX_CONNECTIONS_PER_SERVER = MAX_WORKERS_PER_SERVER + MAX_NUMBER_OF_LOGS_COLLECTORS,
};

typedef struct ClientMetaInfo {
    char host[48];
    char port[16];
    char numeric_host[48];
    char numeric_port[16];
} ClientMetaInfo;

typedef struct Server {
    atomic_int sock_fd;
    struct sockaddr_in sock_addr;

    pthread_mutex_t first_workers_mutex;
    struct sockaddr_in first_workers_addrs[MAX_NUMBER_OF_FIRST_WORKERS];
    int first_workers_fds[MAX_NUMBER_OF_FIRST_WORKERS];
    struct ClientMetaInfo first_workers_info[MAX_NUMBER_OF_FIRST_WORKERS];
    volatile size_t first_workers_arr_size;

    pthread_mutex_t second_workers_mutex;
    struct sockaddr_in second_workers_addrs[MAX_NUMBER_OF_SECOND_WORKERS];
    int second_workers_fds[MAX_NUMBER_OF_SECOND_WORKERS];
    struct ClientMetaInfo second_workers_info[MAX_NUMBER_OF_SECOND_WORKERS];
    volatile size_t second_workers_arr_size;

    pthread_mutex_t third_workers_mutex;
    struct sockaddr_in third_workers_addrs[MAX_NUMBER_OF_THIRD_WORKERS];
    int third_workers_fds[MAX_NUMBER_OF_THIRD_WORKERS];
    struct ClientMetaInfo third_workers_info[MAX_NUMBER_OF_THIRD_WORKERS];
    volatile size_t third_workers_arr_size;

    pthread_mutex_t logs_collectors_mutex;
    struct sockaddr_in logs_collectors_addrs[MAX_NUMBER_OF_LOGS_COLLECTORS];
    int logs_collectors_fds[MAX_NUMBER_OF_LOGS_COLLECTORS];
    struct ClientMetaInfo logs_collectors_info[MAX_NUMBER_OF_LOGS_COLLECTORS];
    volatile size_t logs_collectors_arr_size;

    struct ServerLogsQueue logs_queue;
} Server[1];

bool init_server(Server server, uint16_t server_port);
void deinit_server(Server server);

bool server_accept_client(Server server, ClientType* type, size_t* insert_index);
bool nonblocking_enqueue_log(Server server, const ServerLog* log);
bool dequeue_log(Server server, ServerLog* log);
void send_server_log(Server server, const ServerLog* log);
void send_shutdown_signal_to_one_client(const Server server, ClientType type, size_t index);
void send_shutdown_signal_to_first_workers(Server server);
void send_shutdown_signal_to_second_workers(Server server);
void send_shutdown_signal_to_third_workers(Server server);
void send_shutdown_signal_to_logs_collectors(Server server);
void send_shutdown_signal_to_all_clients_of_type(Server server, ClientType type);
void send_shutdown_signal_to_all(Server server);

bool nonblocking_poll_workers_on_the_first_stage(Server server, PinsQueue pins_1_to_2);
bool nonblocking_poll_workers_on_the_second_stage(Server server, PinsQueue pins_1_to_2, PinsQueue pins_2_to_3);
bool nonblocking_poll_workers_on_the_third_stage(Server server, PinsQueue pins_2_to_3);

static inline bool server_lock_mutex(pthread_mutex_t* mutex);
static inline bool server_unlock_mutex(pthread_mutex_t* mutex);
static inline bool server_lock_first_mutex(Server server);
static inline bool server_unlock_first_mutex(Server server);
static inline bool server_lock_second_mutex(Server server);
static inline bool server_unlock_second_mutex(Server server);
static inline bool server_lock_third_mutex(Server server);
static inline bool server_unlock_third_mutex(Server server);
static inline bool server_lock_logs_collectors_mutex(Server server);
static inline bool server_unlock_logs_collectors_mutex(Server server);
```

Т.к. константа MAX_NUMBER_OF_LOGS_COLLECTORS = 6, то в данном случае возможно подключение нескольких, но не более 6 клиентов-логгеров.

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

### Структура кольцевой очереди сообщений ServerLogsQueue описана в файле "server-logs-queue.h":
```c
enum { SERVER_LOGS_QUEUE_MAX_SIZE = 8 };

struct ServerLogsQueue {
    ServerLog array[SERVER_LOGS_QUEUE_MAX_SIZE];
    size_t size;
    pthread_mutex_t queue_access_mutex;
    sem_t added_elems_sem;
    sem_t free_elems_sem;
    size_t read_index;
    size_t write_index;
};

static inline bool init_server_logs_queue(struct ServerLogsQueue* queue);
static inline void deinit_server_logs_queue(struct ServerLogsQueue* queue);
static inline bool server_logs_queue_nonblocking_enqueue(struct ServerLogsQueue* queue, const ServerLog* log);
static inline bool server_logs_queue_dequeue(struct ServerLogsQueue* queue, ServerLog* log);
```

### Первый клиент (рабочий на 1 участке) парсит входные аргументы, и, если адрес порт указаны корректно, инициализирует ресурсы структуру Client и запускает основной цикл:
```c
static int run_worker(const char* server_ip_address, uint16_t server_port) {
    Client worker;
    if (!init_client(worker, server_ip_address, server_port, FIRST_STAGE_WORKER_CLIENT)) {
        return EXIT_FAILURE;
    }

    print_client_info(worker);
    int ret = start_runtime_loop(worker);
    deinit_client(worker);
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
static int start_runtime_loop(Client worker) {
    int ret = EXIT_SUCCESS;
    while (!client_should_stop(worker)) {
        const Pin pin = receive_new_pin();
        log_received_pin(pin);
        bool is_ok = check_pin_crookness(pin);
        log_checked_pin(pin, is_ok);
        if (!is_ok) {
            continue;
        }

        if (client_should_stop(worker)) {
            break;
        }
        if (!send_not_croocked_pin(worker, pin)) {
            ret = EXIT_FAILURE;
            break;
        }
        log_sent_pin(pin);
    }

    if (ret == EXIT_SUCCESS) {
        printf("Received shutdown signal from the server\n");
    }

    printf("First worker is stopping...\n");
    return ret;
}
```

### Программы second-worker.c и third-worker.c устроены аналогично программе first-worker.c, но эмулируют работу рабочих на 2 и 3 участках соответственно.

### Клиент-логгер парсит входные аргументы, и, если адрес порт указаны корректно, инициализирует ресурсы структуру Client и запускает основной цикл:
```c
static int run_logs_collector(const char* server_ip_address, uint16_t server_port) {
    Client logs_col;
    if (!init_client(logs_col, server_ip_address, server_port, LOGS_COLLECTOR_CLIENT)) {
        return EXIT_FAILURE;
    }

    print_client_info(logs_col);
    int ret = start_runtime_loop(logs_col);
    deinit_client(logs_col);
    return ret;
}

int main(int argc, char const* argv[]) {
    ParseResultCollector res = parse_args_logs_collector(argc, argv);
    if (res.status != PARSE_SUCCESS) {
        print_invalid_args_error_logs_collector(res.status, argv[0]);
        return EXIT_FAILURE;
    }

    return run_logs_collector(res.ip_address, res.port);
}
```

```c
static int start_runtime_loop(Client logs_col) {
    int ret = EXIT_SUCCESS;
    ServerLog log;
    while (!client_should_stop(logs_col)) {
        if (!receive_server_log(logs_col, &log)) {
            ret = EXIT_FAILURE;
            break;
        }

        print_server_log(&log);
    }

    if (ret == EXIT_SUCCESS) {
        printf("Received shutdown signal from the server\n");
    }

    printf("Logs collector is stopping...\n");
    return ret;
}
```

### Интерфейс модуля client-tools.h для работы с сетью (в отчёте без include'ов и у функций только сигнатуры):
```c
typedef struct Client {
    int client_sock_fd;
    ClientType type;
    struct sockaddr_in server_sock_addr;
} Client[1];

bool init_client(Client client, const char* server_ip, uint16_t server_port, ClientType type);
void deinit_client(Client client);

static inline bool is_worker(const Client client);
bool client_should_stop(const Client client);
bool receive_server_log(Client logs_collector, ServerLog* log);
void print_sock_addr_info(const struct sockaddr* address, socklen_t sock_addr_len);
static inline void print_client_info(Client client);

Pin receive_new_pin();
bool check_pin_crookness(Pin pin);
bool send_pin(int sock_fd, const struct sockaddr_in* sock_addr, Pin pin);
bool send_not_croocked_pin(Client worker, Pin pin);
bool receive_pin(int sock_fd, Pin* rec_pin);
bool receive_not_crooked_pin(Client worker, Pin* rec_pin);
void sharpen_pin(Pin pin);
bool send_sharpened_pin(Client worker, Pin pin);
bool receive_sharpened_pin(Client worker, Pin* rec_pin);
bool check_sharpened_pin_quality(Pin sharpened_pin);
```

---
## Пример работы приложений

#### Запуск сервера без аргументов. Сервер сообщает о неверно введённых входных данных и выводит формат и пример использования.
![serv_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-8/images/img1.png?raw=true)

#### Запуск клиенты без аргументов. Клиент сообщает о неверно введённых входных данных и выводит формат и пример использования.
![clnt_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-8/images/img2.png?raw=true)

#### Запуск сервера. Сервер запускается на локальном адресе 127.0.0.1 (localhost) на порте 45235.
![img3](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-8/images/img3.png?raw=true)

#### Запуск клиента first-worker (рабочего на 1 площадке). Клиент подключается к серверу по адресу 127.0.0.1:45234 и выводит информацию о сервере (правая консоль). Сервер принимает клиента и выводит информацию о нём (левая консоль) (информация: "first stage worker"(address=localhost:54100 | 127.0.0.1:54100)). На момент создания скриншота клиент уже успел передать серверу одну проверенную на отсутствие кривизны булавку с id 1804289383. Сервер вывел информацию о том, что получил булавку с id 1804289383.
![img4](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-8/images/img4.png?raw=true)

#### Запуск клиента second-worker (рабочего на 2 площадке). Клиент подключается к серверу по адресу 127.0.0.1:45234 и выводит информацию о сервере (3-я консоль слева на право). Сервер принимает клиента и выводит информацию о нём (1-я консоль слева на право) (информация: "second stage worker"(address=localhost:41310 | 127.0.0.1:41310)). На момент создания скриншота сервер уже успел передать клиенту одну проверенную на отсутствие кривизны булавку с id 1804289383. Клиент вывел информацию о том, что получил булавку с id 1804289383 и начал затачивать её.
![img5](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-8/images/img5.png?raw=true)

#### Запуск клиента third-worker (рабочего на 3 площадке). Клиент подключается к серверу по адресу 127.0.0.1:45235 и выводит информацию о сервере (4-я консоль слева на право). Сервер принимает клиента и выводит информацию о нём (1-я консоль слева на право) (информация: "third stage worker"(address=localhost:56006 | 127.0.0.1:56006)).
![img6](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-8/images/img6.png?raw=true)

#### Запуск первого клиента-логгера logs-collector. Клиент подключается к серверу по адресу 127.0.0.1:45235 и выводит информацию о сервере (а также логи, отправленные сервером) (5-я консоль слева на право). Сервер принимает клиента и выводит информацию о нём (1-я консоль слева на право) (информация: ""(address=localhost:49596 | 127.0.0.1:49596)).
![img7](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-8/images/img7.png?raw=true)

#### Запуск второго клиента-логгера logs-collector. Клиент подключается к серверу по адресу 127.0.0.1:45235 и выводит информацию о сервере (а также логи, отправленные сервером) (6-я консоль слева на право). Сервер принимает клиента и выводит информацию о нём (1-я консоль слева на право) (информация: ""(address=localhost:46484 | 127.0.0.1:46484)).
![img8](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-8/images/img8.png?raw=true)

#### Завершение работы системы при нажатии сочетания клавиш Ctrl-C в консоли сервера. Сервер посылает сигнал о завершении клиентам, ждёт некоторое время, и после этого закрывает все сокеты и деинициализирует ресурсы. Все клиенты и сервер завершили работу.
![img9](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-8/images/img9.png?raw=true)
