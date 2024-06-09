# Отчёт о выполении ИДЗ №4

кормилицын владимир алексеевич, БПИ226

## Описание программ

**Для выполнения задачи были написаны 5 программ:**
* #### server.c - сервер, к которому подключаются клиенты (рабочие на участках) и который перенаправляет сообщения между ними, выводя информацию в консоль. Функции для инициализации стркутуры Server и работы с сетью вынесены в модуль server-tools.c.
* #### first-worker.c - клиент, эмулирующий работу работника на 1 участке.
* #### second-worker.c - клиент, эмулирующий работу работника на 2 участке.
* #### third-worker.c - клиент, эмулирующий работу работника на 3 участке.
* #### logs-collector.c - клиент, эмулирующий работу работника на 3 участке.
#### Для работы с сетью все клиенты используют функции из модуля client-tools.c, сервер - из модуля server-tools.c.

Корретное завершение работы приложения возможно при нажатии сочетания клавиш Ctrl-C, в таком случае сервер пошлёт всем клиентам специальный сигнал о завершении работы и корректно деинициализирует все выделенные ресурсы. Если клиент разрывает соединение, сервер корректно обрабатывает это и удаляет клиента из внуреннего массива клиентов.

---

### Сервер парсит входные аргументы, и, если порт указан верно, создаёт и инициализирует ресурсы, создаёт процесс, который будет опрашивать клиентом, и процесс, рассылающий логи, а в основном процессе ждёт новых клиентов.

```c
static int run_server(uint16_t server_port) {
    if (!init_server(&server, server_port)) {
        return EXIT_FAILURE;
    }

    int ret = start_runtime_loop();
    deinit_server(&server);
    printf("> Deinitialized server resources\n");
    return ret;
}

int main(int argc, const char* argv[]) {
    setup_signal_handler();

    ParseResult res = parse_args(argc, argv);
    if (res.status != PARSE_SUCCESS) {
        print_invalid_args_error(res.status, argv[0]);
        return EXIT_FAILURE;
    }

    return run_server(res.port);
}
```

```c
static int start_runtime_loop(void) {
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

    const int ret_poller = join_thread(poll_thread);
    printf("> Joined polling thread\n");
    const int ret_logger = join_thread(logs_thread);
    printf("> Joined logging thread\n");

    printf("> Started sending shutdown signals to all clients\n");
    send_shutdown_signal_to_all(&server);
    printf("> Sent shutdown signals to all clients\n");

    return ret_poller | ret_logger;
}
```

### Процесс-поллер раз в некоторое время оправшивает клиентов:
```c
static void* workers_poller(void* unused) {
    (void)unused;

    while (is_poller_running) {
        if (!nonblocking_poll(&server)) {
            fprintf(stderr, "> Could not poll clients\n");
            break;
        }
        if (!app_thread_sleep()) {
            break;
        }
    }

    int32_t ret = is_poller_running ? EXIT_FAILURE : EXIT_SUCCESS;
    stop_all_threads();
    return (void*)(uintptr_t)(uint32_t)ret;
}
```

### Процесс-логгер ожидает логи из очереди логов (паттерн очереди, поддерживающей работу с несколькими процессами - producer-consumer с 1 мьютексом и 2 семафорами) и отправляет их всем подключённым клиентам-логгерам:
```c
static void* logs_sender(void* unused) {
    (void)unused;

    ServerLog log = {0};
    while (is_logger_running) {
        if (!dequeue_log(&server, &log)) {
            fputs("> Could not get next log\n", stderr);
            break;
        }
        if (!send_server_log(&server, &log)) {
            fputs("> Could not send log\n", stderr);
            break;
        }
        if (!app_thread_sleep()) {
            break;
        }
    }

    int32_t ret = is_logger_running ? EXIT_FAILURE : EXIT_SUCCESS;
    stop_all_threads();
    return (void*)(uintptr_t)(uint32_t)ret;
}
```

### Интерфейс модуля server-tools.h для работы с сетью (в отчёте без include'ов и у функций только сигнатуры):
```c
enum {
    MAX_NUMBER_OF_FIRST_WORKERS  = 3,
    MAX_NUMBER_OF_SECOND_WORKERS = 5,
    MAX_NUMBER_OF_THIRD_WORKERS  = 2,

    MAX_WORKERS_PER_SERVER =
        MAX_NUMBER_OF_FIRST_WORKERS + MAX_NUMBER_OF_SECOND_WORKERS + MAX_NUMBER_OF_THIRD_WORKERS,
    MAX_CONNECTIONS_PER_SERVER = MAX_WORKERS_PER_SERVER
};

typedef struct Server {
    int sock_fd;
    struct sockaddr_in sock_addr;
    struct ServerLogsQueue logs_queue;
} Server[1];

bool init_server(Server server, uint16_t server_port);
void deinit_server(Server server);
bool nonblocking_poll(Server server);
void send_shutdown_signal_to_all(const Server server);

bool nonblocking_enqueue_log(Server server, const ServerLog* log);
bool dequeue_log(Server server, ServerLog* log);
bool send_server_log(const Server server, const ServerLog* log);
```

### Структура булавки описана в файле "pin.h":

```c
/// @brief Pin that workers pass to each other.
typedef struct Pin {
    int pin_id;
} Pin;
```

### Структура очереди логов описана в файле "server-logs-queue.h"
```c
enum { SERVER_LOGS_QUEUE_MAX_SIZE = 16 };

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

### Первый клиент (рабочий на 1 участке) парсит входные аргументы, и, если порт указан корректно, инициализирует ресурсы структуру Client и запускает основной цикл:
```c
static int run_worker(uint16_t server_port) {
    Client worker;
    if (!init_client(worker, server_port, COMPONENT_TYPE_FIRST_STAGE_WORKER)) {
        return EXIT_FAILURE;
    }

    print_client_info(worker);
    int ret = start_runtime_loop(worker);
    deinit_client(worker);
    return ret;
}

int main(int argc, const char* argv[]) {
    ParseResult res = parse_args(argc, argv);
    if (res.status != PARSE_SUCCESS) {
        print_invalid_args_error(res.status, argv[0]);
        return EXIT_FAILURE;
    }

    return run_worker(res.port);
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
        printf(
            "+------------------------------------------+\n"
            "| Received shutdown signal from the server |\n"
            "+------------------------------------------+\n");
    }

    printf(
        "+-----------------------------+\n"
        "| First worker is stopping... |\n"
        "+-----------------------------+\n");
    return ret;
}
```

### Программы second-worker.c и third-worker.c устроены аналогично программе first-worker.c, но эмулируют работу рабочих на 2 и 3 участках соответственно.

### Клиент для отображения информации о работе системы (клиент, получающий логи от сервера) парсит входные аргументы, и, если порт указан корректно, инициализирует ресурсы структуру Client и запускает основной цикл:

```c
static int run_logs_collector(uint16_t server_port) {
    Client logs_col;
    if (!init_client(logs_col, server_port, COMPONENT_TYPE_LOGS_COLLECTOR)) {
        return EXIT_FAILURE;
    }

    print_client_info(logs_col);
    int ret = start_runtime_loop(logs_col);
    deinit_client(logs_col);
    return ret;
}

int main(int argc, char const* argv[]) {
    ParseResult res = parse_args(argc, argv);
    if (res.status != PARSE_SUCCESS) {
        print_invalid_args_error(res.status, argv[0]);
        return EXIT_FAILURE;
    }

    return run_logs_collector(res.port);
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
        printf(
            "+------------------------------------------+\n"
            "| Received shutdown signal from the server |\n"
            "+------------------------------------------+\n");
    }

    printf(
        "+-------------------------------+\n"
        "| Logs collector is stopping... |\n"
        "+-------------------------------+\n");
    return ret;
}
```

### Интерфейс модуля client-tools.h для работы с сетью (в отчёте без include'ов и у функций только сигнатуры):
```c
typedef struct Client {
    int client_sock_fd;
    ComponentType type;
    struct sockaddr_in server_broadcast_sock_addr;
} Client[1];

bool init_client(Client client, uint16_t server_port, ComponentType type);
void deinit_client(Client client);

static inline bool is_worker(const Client client) {
    switch (client->type) {
        case COMPONENT_TYPE_FIRST_STAGE_WORKER:
        case COMPONENT_TYPE_SECOND_STAGE_WORKER:
        case COMPONENT_TYPE_THIRD_STAGE_WORKER:
            return true;
        default:
            return false;
    }
}

bool client_should_stop(const Client client);
void print_sock_addr_info(const struct sockaddr* address, socklen_t sock_addr_len);
static inline void print_client_info(const Client client) {
    print_sock_addr_info((const struct sockaddr*)&client->server_broadcast_sock_addr,
                         sizeof(client->server_broadcast_sock_addr));
}
Pin receive_new_pin(void);
bool check_pin_crookness(Pin pin);
bool send_not_croocked_pin(const Client worker, Pin pin);
bool receive_not_crooked_pin(const Client worker, Pin* rec_pin);
void sharpen_pin(Pin pin);
bool send_sharpened_pin(const Client worker, Pin pin);
bool receive_sharpened_pin(const Client worker, Pin* rec_pin);
bool check_sharpened_pin_quality(Pin sharpened_pin);
bool receive_server_log(Client logs_collector, ServerLog* log);
```

---
## Пример работы приложений

#### Запуск сервера без аргументов. Сервер сообщает о неверно введённых входных данных и выводит формат и пример использования.
![serv_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw4/mark-8/images/img1.png?raw=true)

#### Запуск клиента без аргументов. Клиент сообщает о неверно введённых входных данных и выводит формат и пример использования.
![clnt_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw4/mark-8/images/img2.png?raw=true)

#### Запуск сервера. Сервер запускается на локальном адресе на порте 45592.
![img3](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw4/mark-8/images/img3.png?raw=true)

#### Запуск клиента first-worker (рабочего на 1 площадке). Клиент подключается к серверу и выводит информацию о сервере (правая консоль). Сервер принимает клиента и выводит информацию о нём (левая консоль) (информация: "first stage worker").
![img4](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw4/mark-8/images/img4.png?raw=true)

#### Запуск клиента second-worker (рабочего на 2 площадке). Клиент подключается к серверу и выводит информацию о сервере (3-я консоль слева на право). Сервер принимает клиента и выводит информацию о нём (1-я консоль слева на право) (информация: "second stage worker").
![img5](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw4/mark-8/images/img5.png?raw=true)

#### Запуск клиента third-worker (рабочего на 3 площадке). Клиент подключается к серверу и выводит информацию о сервере (4-я консоль слева на право). Сервер принимает клиента и выводит информацию о нём (1-я консоль слева на право) (информация: "third stage worker").
![img6](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw4/mark-8/images/img6.png?raw=true)

#### Запуск первого клиента logs-collector (клиент, получающий логи от сервера). Клиент подключается к серверу и выводит информацию о сервере (5-я консоль слева на право). Сервер принимает клиента и выводит информацию о нём (1-я консоль слева на право) (информация: "logs collector").
![img7](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw4/mark-8/images/img7.png?raw=true)

#### Запуск второго клиента logs-collector (клиент, получающий логи от сервера). Клиент подключается к серверу и выводит информацию о сервере (6-я консоль слева на право). Сервер принимает клиента и выводит информацию о нём (1-я консоль слева на право) (информация: "logs collector").
![img8](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw4/mark-8/images/img8.png?raw=true)

#### Завершение работы системы при нажатии сочетания клавиш Ctrl-C в консоли сервера. Сервер посылает сигнал о завершении клиентам и закрывает все сокеты и деинициализирует ресурсы. Все клиенты и сервер завершили работу.
![img9](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw4/mark-8/images/img9.png?raw=true)
