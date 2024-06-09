# Отчёт о выполении ИДЗ №4

кормилицын владимир алексеевич, БПИ226

## Описание программ

**Для выполнения задачи были написаны 4 программы:**
* #### server.c - сервер, к которому подключаются клиенты (рабочие на участках) и который перенаправляет сообщения между ними, выводя информацию в консоль. Функции для инициализации стркутуры Server и работы с сетью вынесены в модуль server-tools.c.
* #### first-worker.c - клиент, эмулирующий работу работника на 1 участке.
* #### second-worker.c - клиент, эмулирующий работу работника на 2 участке.
* #### third-worker.c - клиент, эмулирующий работу работника на 3 участке.
#### Для работы с сетью все клиенты используют функции из модуля client-tools.c, сервер - из модуля server-tools.c.

Корретное завершение работы приложения возможно при нажатии сочетания клавиш Ctrl-C, в таком случае сервер пошлёт всем клиентам специальный сигнал о завершении работы и корректно деинициализирует все выделенные ресурсы. Если клиент разрывает соединение, сервер корректно обрабатывает это и удаляет клиента из внуреннего массива клиентов.

---

### Типы компонентов программы описываются перечислением ComponentType, сообщение, пересылаемое между ними, - структурой UDPMessage, тип сообщения - перечислением MessageType:

```c
typedef enum ComponentType {
    COMPONENT_TYPE_SERVER              = 1u << 0,
    COMPONENT_TYPE_FIRST_STAGE_WORKER  = 1u << 1,
    COMPONENT_TYPE_SECOND_STAGE_WORKER = 1u << 2,
    COMPONENT_TYPE_THIRD_STAGE_WORKER  = 1u << 3,
    COMPONENT_TYPE_ANY_WORKER          = COMPONENT_TYPE_FIRST_STAGE_WORKER |
                                COMPONENT_TYPE_SECOND_STAGE_WORKER |
                                COMPONENT_TYPE_THIRD_STAGE_WORKER
} ComponentType;

typedef enum MessageType {
    MESSAGE_TYPE_PIN_TRANSFERRING,
    MESSAGE_TYPE_NEW_CLIENT,
    MESSAGE_TYPE_SHUTDOWN_MESSAGE,
} MessageType;

enum {
    UDP_MESSAGE_SIZE          = 512,
    UDP_MESSAGE_METAINFO_SIZE = 2 * sizeof(ComponentType) + sizeof(MessageType),
    UDP_MESSAGE_BUFFER_SIZE   = UDP_MESSAGE_SIZE - UDP_MESSAGE_METAINFO_SIZE,
};

typedef struct UDPMessage {
    ComponentType sender_type;
    ComponentType receiver_type;
    MessageType message_type;
    union {
        Pin pin;
        char bytes[UDP_MESSAGE_BUFFER_SIZE];
    } message_content;
} UDPMessage;
```

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
    const int ret = join_thread(poll_thread);
    printf("> Joined polling thread\n");

    printf("> Started sending shutdown signals to all clients\n");
    send_shutdown_signal_to_all(&server);
    printf("> Sent shutdown signals to all clients\n");

    return ret;
}
```

### Процесс-поллер раз в некоторое время оправшивает клиентов:
```c
static void* workers_poller(void* unused) {
    (void)unused;
    const struct timespec sleep_time = {
        .tv_sec  = 1,
        .tv_nsec = 500000000,
    };

    while (is_poller_running) {
        if (!nonblocking_poll(&server)) {
            fprintf(stderr, "> Could not poll clients\n");
            break;
        }
        if (nanosleep(&sleep_time, NULL) == -1) {
            if (errno != EINTR) {
                // if not interrupted by the signal
                perror("nanosleep");
            }
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
} Server[1];

bool init_server(Server server, uint16_t server_port);
void deinit_server(Server server);
bool nonblocking_poll(const Server server);
void send_shutdown_signal_to_all(const Server server);
```

### Структура булавки описана в файле "pin.h":

```c
/// @brief Pin that workers pass to each other.
typedef struct Pin {
    int pin_id;
} Pin;
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
```

### Программы second-worker.c и third-worker.c устроены аналогично программе first-worker.c, но эмулируют работу рабочих на 2 и 3 участках соответственно.

---
## Пример работы приложений

#### Запуск сервера без аргументов. Сервер сообщает о неверно введённых входных данных и выводит формат и пример использования.
![serv_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw4/mark-4-5/images/img1.png?raw=true)

#### Запуск клиента без аргументов. Клиент сообщает о неверно введённых входных данных и выводит формат и пример использования.
![clnt_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw4/mark-4-5/images/img2.png?raw=true)

#### Запуск сервера. Сервер запускается на локальном адресе на порте 45592.
![img3](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw4/mark-4-5/images/img3.png?raw=true)

#### Запуск клиента first-worker (рабочего на 1 площадке). Клиент подключается к серверу и выводит информацию о сервере (правая консоль). Сервер принимает клиента и выводит информацию о нём (левая консоль) (информация: "first stage worker").
![img4](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw4/mark-4-5/images/img4.png?raw=true)

#### Запуск клиента second-worker (рабочего на 2 площадке). Клиент подключается к серверу и выводит информацию о сервере (3-я консоль слева на право). Сервер принимает клиента и выводит информацию о нём (1-я консоль слева на право) (информация: "second stage worker").
![img5](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw4/mark-4-5/images/img5.png?raw=true)

#### Запуск клиента third-worker (рабочего на 3 площадке). Клиент подключается к серверу по адресу 127.0.0.1:45235 и выводит информацию о сервере (4-я консоль слева на право). Сервер принимает клиента и выводит информацию о нём (1-я консоль слева на право) (информация: "third stage worker").
![img6](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw4/mark-4-5/images/img6.png?raw=true)

#### Завершение работы системы при нажатии сочетания клавиш Ctrl-C в консоли сервера. Сервер посылает сигнал о завершении клиентам и закрывает все сокеты и деинициализирует ресурсы. Все клиенты и сервер завершили работу.
![img7](https://github.com/i80287/arch-comp-systems/blob/main/os_ihw4/mark-4-5/images/img7.png?raw=true)
