# Отчёт о выполении ИДЗ №3

кормилицын владимир алексеевич, БПИ226

## Описание программ

**Для выполнения задачи были написаны 6 программ:**
* #### server.c - сервер, к которому подключаются клиенты (рабочие на участках) и который перенаправляет сообщения между ними, выводя информацию в консоль. Функции для инициализации стркутуры Server и работы с сетью вынесены в модуль server-tools.c.
* #### first-worker.c - клиент, эмулирующий работу работника на 1 участке.
* #### second-worker.c - клиент, эмулирующий работу работника на 2 участке.
* #### third-worker.c - клиент, эмулирующий работу работника на 3 участке.
* #### logs-collector.c - клиент, подключающийся к серверу и получающий логи от него.
* #### manager.c - клиент, подключающийся к серверу и имеющий возможность отключать других клиентов.
#### Для работы с сетью все клиенты используют функции из модуля client-tools.c.

Корретное завершение работы приложения возможно при нажатии сочетания клавиш Ctrl-C, в таком случае сервер пошлёт всем клиентам специальный сигнал о завершении работы и корректно деинициализирует все выделенные ресурсы. Если клиент разрывает соединение, сервер корректно обрабатывает это и удаляет клиента из внуреннего массива клиентов.

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
    if (!create_thread(&poll_thread, &workers_poller)) {
        return EXIT_FAILURE;
    }
    printf("> Started polling thread\n");

    pthread_t logs_thread;
    if (!create_thread(&logs_thread, &logs_sender)) {
        pthread_cancel(poll_thread);
        return EXIT_FAILURE;
    }
    printf("> Started logging thread\n");

    pthread_t managers_thread;
    if (!create_thread(&managers_thread, &managers_handler)) {
        pthread_cancel(logs_thread);
        pthread_cancel(poll_thread);
        return EXIT_FAILURE;
    }
    printf("> Started managers thread\n");

    printf("> Server ready to accept connections\n");
    while (is_acceptor_running) {
        if (!server_accept_client(&server)) {
            break;
        }
    }

    const int ret = is_acceptor_running ? EXIT_FAILURE : EXIT_SUCCESS;
    if (is_acceptor_running) {
        stop_all_threads();
    }
    const int ret_1 = join_thread(logs_thread);
    printf("Joined logging thread\n");
    const int ret_2 = join_thread(poll_thread);
    printf("Joined polling thread\n");
    const int ret_3 = join_thread(managers_thread);
    printf("Joined managers thread\n");

    printf("> Started sending shutdown signals to all clients\n");
    send_shutdown_signal_to_all(&server);
    printf(
        "> Sent shutdown signals to all clients\n"
        "> Started waiting for %u seconds before closing the sockets\n",
        (uint32_t)MAX_SLEEP_TIME);
    sleep(MAX_SLEEP_TIME);

    return ret | ret_1 | ret_2 | ret_3;
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

    int32_t ret = is_poller_running ? EXIT_FAILURE : EXIT_SUCCESS;
    if (is_poller_running) {
        stop_all_threads();
    }
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

### Процесс, работающий с менеджерами клиентов (клиенты, которые могу отключать других клиентов и себя), раз в некоторое время оправшиает клиентов на наличие команд на исполнение, и, если команда есть, выполняет её и посылает результат выполнения обратно клиенту, отправившему команду.
```c
static void* managers_handler(void* unused) {
    (void)unused;
    const struct timespec sleep_time = {
        .tv_sec  = 1,
        .tv_nsec = 500000000,
    };

    ServerLog log;
    ServerCommand cmd = {0};
    while (is_managers_handler_running) {
        if (nanosleep(&sleep_time, NULL) == -1) {
            if (errno != EINTR) {  // if not interrupted by the signal
                perror("nanosleep");
            }
            break;
        }
        size_t manager_index = (size_t)-1;
        if (!nonblocking_poll_managers(&server, &cmd, &manager_index)) {
            fputs("> Could not get next command\n", stderr);
            break;
        }
        if (manager_index == (size_t)-1) {
            continue;
        }
        const ClientMetaInfo* const info = &server.managers_info[manager_index];
        int ret = snprintf(log.message, sizeof(log.message),
                           "> Received command to shutdown "
                           "client[address=%s:%u]\n"
                           "  from manager[address=%s:%s | %s:%s]\n",
                           cmd.ip_address, cmd.port, info->host, info->port, info->numeric_host,
                           info->numeric_port);
        assert(ret > 0);
        assert(log.message[0] != '\0');
        print_and_enqueue_log(&log);
        ServerCommandResult res = execute_command(&server, &cmd);
        ret = snprintf(log.message, sizeof(log.message),
                       "> Executed shutdown command on "
                       "client at address %s:%u\n"
                       "  Server result: %s\n",
                       cmd.ip_address, cmd.port, server_command_result_to_string(res));
        assert(ret > 0);
        assert(log.message[0] != '\0');
        print_and_enqueue_log(&log);
        if (!send_command_result_to_manager(&server, res, manager_index)) {
            fputs("> Could not send command result to manger\n", stderr);
            break;
        }
        ret = snprintf(log.message, sizeof(log.message),
                       "> Send command result back to "
                       "manager[address=%s:%s | %s:%s]\n",
                       info->host, info->port, info->numeric_host, info->numeric_port);
        assert(ret > 0);
        assert(log.message[0] != '\0');
        print_and_enqueue_log(&log);
    }

    int32_t ret = is_managers_handler_running ? EXIT_FAILURE : EXIT_SUCCESS;
    if (is_managers_handler_running) {
        stop_all_threads();
    }
    return (void*)(uintptr_t)(uint32_t)ret;
}
```

### Интерфейс модуля server-tools.h для работы с сетью (в отчёте без include'ов и у функций только сигнатуры):
```c
#pragma once

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <errno.h>       // for errno
#include <netinet/in.h>  // for sockaddr_in
#include <pthread.h>     // for pthread_mutex_t, pthread_mutex_lock, pthre...
#include <stdatomic.h>   // for atomic_int
#include <stdbool.h>     // for bool
#include <stdint.h>      // for uint16_t
#include <stdio.h>       // for size_t, perror
#include <stdlib.h>      // for rand

#include "net-config.h"
#include "server-command.h"
#include "server-log.h"
#include "server-logs-queue.h"

enum {
    MAX_NUMBER_OF_FIRST_WORKERS   = 3,
    MAX_NUMBER_OF_SECOND_WORKERS  = 5,
    MAX_NUMBER_OF_THIRD_WORKERS   = 2,
    MAX_NUMBER_OF_LOGS_COLLECTORS = 6,
    MAX_NUMBER_OF_MANAGERS        = 4,

    MAX_WORKERS_PER_SERVER =
        MAX_NUMBER_OF_FIRST_WORKERS + MAX_NUMBER_OF_SECOND_WORKERS + MAX_NUMBER_OF_THIRD_WORKERS,
    MAX_CONNECTIONS_PER_SERVER =
        MAX_WORKERS_PER_SERVER + MAX_NUMBER_OF_LOGS_COLLECTORS + MAX_NUMBER_OF_MANAGERS,
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

    pthread_mutex_t managers_mutex;
    struct sockaddr_in managers_addrs[MAX_CONNECTIONS_PER_SERVER];
    int managers_fds[MAX_CONNECTIONS_PER_SERVER];
    struct ClientMetaInfo managers_info[MAX_CONNECTIONS_PER_SERVER];
    volatile size_t managers_arr_size;

    struct ServerLogsQueue logs_queue;
} Server[1];

bool init_server(Server server, uint16_t server_port);
void deinit_server(Server server);

bool server_accept_client(Server server);
bool nonblocking_enqueue_log(Server server, const ServerLog* log);
bool dequeue_log(Server server, ServerLog* log);
void send_server_log(Server server, const ServerLog* log);
void send_shutdown_signal_to_one_client(const Server server, ClientType type, size_t index);
void send_shutdown_signal_to_first_workers(Server server);
void send_shutdown_signal_to_second_workers(Server server);
void send_shutdown_signal_to_third_workers(Server server);
void send_shutdown_signal_to_logs_collectors(Server server);
void send_shutdown_signal_to_managers(Server server);
void send_shutdown_signal_to_all_clients_of_type(Server server, ClientType type);
void send_shutdown_signal_to_all(Server server);

bool nonblocking_poll_workers_on_the_first_stage(Server server, PinsQueue pins_1_to_2);
bool nonblocking_poll_workers_on_the_second_stage(Server server, PinsQueue pins_1_to_2,
                                                  PinsQueue pins_2_to_3);
bool nonblocking_poll_workers_on_the_third_stage(Server server, PinsQueue pins_2_to_3);
bool nonblocking_poll_managers(Server server, ServerCommand* cmd, size_t* manager_index);

ServerCommandResult execute_command(Server server, const ServerCommand* cmd);
bool send_command_result_to_manager(Server server, ServerCommandResult res, size_t manager_index);

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
static inline bool server_lock_managers_mutex(Server server);
static inline bool server_unlock_managers_mutex(Server server);
```

Т.к. константа MAX_NUMBER_OF_LOGS_COLLECTORS = 6, то в данном случае возможно подключение нескольких, но не более 6 клиентов-логгеров.

Т.к. константа MAX_NUMBER_OF_MANAGERS = 6, то в данном случае возможно подключение нескольких, но не более 4 клиентов-менеджеров.

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

### Клиент-менеджер парсит входные аргументы, и, если адрес порт указаны корректно, инициализирует ресурсы структуру Client и запускает основной цикл:

### Интерфейс модуля client-tools.h для работы с сетью (в отчёте без include'ов и у функций только сигнатуры):
```c
static int run_manager(const char* server_ip_address, uint16_t server_port) {
    Client manager;
    if (!init_client(manager, server_ip_address, server_port, MANAGER_CLIENT)) {
        return EXIT_FAILURE;
    }

    print_client_info(manager);
    int ret = start_runtime_loop(manager);
    deinit_client(manager);
    return ret;
}

int main(int argc, char const* argv[]) {
    ParseResultClient res = parse_args_client(argc, argv);
    if (res.status != PARSE_SUCCESS) {
        print_invalid_args_error_client(res.status, argv[0]);
        return EXIT_FAILURE;
    }

    return run_manager(res.ip_address, res.port);
}
```

```c
static bool next_user_command();
static void get_command_args(ServerCommand* cmd);
static void handle_server_response(ServerCommandResult res, int* ret);

static int start_runtime_loop(Client manager) {
    int ret                     = EXIT_SUCCESS;
    bool exit_requested_by_user = false;
    while (ret == EXIT_SUCCESS && !client_should_stop(manager)) {
        if (!next_user_command()) {
            exit_requested_by_user = true;
            break;
        }

        ServerCommand cmd = {0};
        get_command_args(&cmd);
        ServerCommandResult resp = send_command_to_server(manager, &cmd);
        handle_server_response(resp, &ret);
    }

    if (ret == EXIT_SUCCESS && !exit_requested_by_user) {
        printf("Received shutdown signal from the server\n");
    }

    printf("Manager is stopping...\n");
    return ret;
}
```

---
## Пример работы приложений

#### Запуск сервера без аргументов. Сервер сообщает о неверно введённых входных данных и выводит формат и пример использования.
![serv_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-10/images/img1.png?raw=true)

#### Запуск клиенты без аргументов. Клиент сообщает о неверно введённых входных данных и выводит формат и пример использования.
![clnt_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-10/images/img2.png?raw=true)

#### Запуск сервера. Сервер запускается на локальном адресе 127.0.0.1 (localhost) на порте 45235.
![img3](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-10/images/img3.png?raw=true)

#### Запуск клиента first-worker (рабочего на 1 площадке). Клиент подключается к серверу по адресу 127.0.0.1:45234 и выводит информацию о сервере (правая консоль). Сервер принимает клиента и выводит информацию о нём (левая консоль) (информация: "first stage worker"(address=localhost:37116 | 127.0.0.1:37116)). На момент создания скриншота клиент уже успел передать серверу одну проверенную на отсутствие кривизны булавку с id 1804289383. Сервер вывел информацию о том, что получил булавку с id 1804289383.
![img4](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-10/images/img4.png?raw=true)

#### Запуск клиента second-worker (рабочего на 2 площадке). Клиент подключается к серверу по адресу 127.0.0.1:45234 и выводит информацию о сервере (3-я консоль слева на право). Сервер принимает клиента и выводит информацию о нём (1-я консоль слева на право) (информация: "second stage worker"(address=localhost:56154 | 127.0.0.1:56154)). На момент создания скриншота сервер уже успел передать клиенту одну проверенную на отсутствие кривизны булавку с id 1804289383. Клиент вывел информацию о том, что получил булавку с id 1804289383 и начал затачивать её.
![img5](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-10/images/img5.png?raw=true)

#### Запуск клиента third-worker (рабочего на 3 площадке). Клиент подключается к серверу по адресу 127.0.0.1:45235 и выводит информацию о сервере (4-я консоль слева на право). Сервер принимает клиента и выводит информацию о нём (1-я консоль слева на право) (информация: "third stage worker"(address=localhost:46956 | 127.0.0.1:46956)).
![img6](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-10/images/img6.png?raw=true)

#### Запуск клиента-логгера logs-collector. Клиент подключается к серверу по адресу 127.0.0.1:45235 и выводит информацию о сервере (а также логи, отправленные сервером) (5-я консоль слева на право). Сервер принимает клиента и выводит информацию о нём (1-я консоль слева на право) (информация: "logs collector"(address=localhost:45044 | 127.0.0.1:45044)).
![img7](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-10/images/img7.png?raw=true)

#### Запуск клиента-менеджера manager. Клиент подключается к серверу по адресу 127.0.0.1:45235 и выводит информацию о сервере (6-я консоль слева на право). Сервер принимает клиента и выводит информацию о нём (1-я консоль слева на право) (информация: "manager"(address=localhost:60290 | 127.0.0.1:60290)).
![img8](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-10/images/img8.png?raw=true)

#### Клиент-менеджер завершил работу клиента рабочего на 3-й площадке.
![img9](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-10/images/img9.png?raw=true)

#### Клиент рабочего на 3-й площадке third-worker переподключился.
![img10](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-10/images/img10.png?raw=true)

#### Завершение работы системы при нажатии сочетания клавиш Ctrl-C в консоли сервера. Сервер посылает сигнал о завершении клиентам, ждёт некоторое время, и после этого закрывает все сокеты и деинициализирует ресурсы. Все клиенты и сервер завершили работу.
![img11](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw3/mark-10/images/img11.png?raw=true)
