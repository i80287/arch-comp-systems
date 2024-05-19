кормилицын владимир алексеевич, БПИ226

## Описание программ

**Для выполнения задачи были написаны 2 программы:**
* #### server.c - сервер, считывающий текст из стандартного потока ввода и отправляющий их клиентам, слушающим сервер
* #### client.c - клиент, принимающий сообщения от сервера и выводящий их в стандартный поток вывода

#### Сервер парсит входные аргументы, и, если они введены корректно, создаёт и настраивает сокет (в функции `create_server_socket`)

```c
static int run_server(const char* server_ip, uint16_t server_port) {
    int sock_fd = create_server_socket();
    if (sock_fd == -1) {
        return EXIT_FAILURE;
    }
    const struct sockaddr_in broadcast_address = {
        .sin_family      = AF_INET,
        .sin_port        = htons(server_port),
        .sin_addr.s_addr = inet_addr(server_ip),
    };
    int ret = run_server_impl(sock_fd, broadcast_address);
    close(sock_fd);
    return ret;
}

int main(int argc, const char* argv[]) {
    ParseResult res = parse_args(argc, argv);
    if (!res.succeeded) {
        report_invalid_command_args(argv[0]);
        return EXIT_FAILURE;
    }

    return run_server(res.server_ip, res.port);
}
```

#### Функция для создания и настройки сокета:
```c
static inline int create_server_socket() {
    int server_sock_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (server_sock_fd == -1) {
        perror("socket");
        return -1;
    }

    /* Set socket to allow broadcast */
    const int broadcast_permission = 1;
    int setsockopt_res =
        setsockopt(server_sock_fd, SOL_SOCKET, SO_BROADCAST, (const void*)&broadcast_permission,
                   sizeof(broadcast_permission));

    if (setsockopt_res == -1) {
        perror("setsockopt");
        close(server_sock_fd);
        return -1;
    }

    return server_sock_fd;
}
```

#### Функция для :
```c
```

#### Клиент парсит входные аргументы и, если они введены корректно, создаёт сокет и ожидает сообщения от сервера:
```c
static int run_client(uint16_t client_port) {
    int sock_fd = create_client_socket(client_port);
    if (sock_fd == -1) {
        return EXIT_FAILURE;
    }
    int res = run_client_impl(sock_fd);
    close(sock_fd);
    return res;
}

int main(int argc, const char* argv[]) {
    uint16_t client_port;
    if (argc != 2 || !parse_port(argv[1], &client_port)) {
        report_invalid_command_args(argv[0]);
        return EXIT_FAILURE;
    }

    return run_client(client_port);
}
```

#### Функция для создания сокета:
```c
static inline int create_server_socket() {
    int server_sock_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (server_sock_fd == -1) {
        perror("socket");
        return -1;
    }

    /* Set socket to allow broadcast */
    const int broadcast_permission = 1;
    int setsockopt_res =
        setsockopt(server_sock_fd, SOL_SOCKET, SO_BROADCAST, (const void*)&broadcast_permission,
                   sizeof(broadcast_permission));

    if (setsockopt_res == -1) {
        perror("setsockopt");
        close(server_sock_fd);
        return -1;
    }

    return server_sock_fd;
}
```

#### Функция для:
```c

```

#### Алгоритм работы сервера, посылающего считанные сообщения клиентам:
```c
static int run_server_impl(int server_sock_fd, const struct sockaddr_in server_address) {
    char message_buffer[MESSAGE_BUFFER_SIZE] = {0};
    bool received_end_message                = false;
    while (!received_end_message && fgets(message_buffer, MESSAGE_BUFFER_SIZE, stdin)) {
        size_t message_size = strlen(message_buffer);
        if (message_buffer[message_size - 1] == '\n') {
            message_buffer[message_size - 1] = '\0';
            message_size--;
        }

        printf("Transfering message '%s' of length %zu from the first client to the second one\n",
               message_buffer, message_size);
        received_end_message = is_end_message(message_buffer, message_size);

        size_t send_size = message_size + 1;
        if (sendto(server_sock_fd, message_buffer, send_size, 0,
                   (const struct sockaddr*)&server_address,
                   sizeof(server_address)) != (ssize_t)send_size) {
            perror("send");
            break;
        }
    }

    printf("Server is stopping...\n");
    return received_end_message ? EXIT_SUCCESS : EXIT_FAILURE;
}
```

#### Алгоритм работы клиента, печатающего сообщения, полученные от сервера:
```c
static int run_client_impl(int sock_fd) {
    char message_buffer[MESSAGE_BUFFER_SIZE] = {0};
    bool received_end_message                = false;
    do {
        ssize_t received_size =
            recvfrom(sock_fd, message_buffer, MESSAGE_BUFFER_SIZE, 0, NULL, NULL);
        if (received_size <= 0) {
            perror("recvfrom");
            break;
        }

        const size_t message_string_length    = (size_t)received_size - 1;
        message_buffer[message_string_length] = '\0';
        received_end_message = is_end_message(message_buffer, message_string_length);
        printf("Received message '%s' of length %zu from the server\n", message_buffer,
               message_string_length);
    } while (!received_end_message);

    printf("Client is stopping...\n");
    return received_end_message ? EXIT_SUCCESS : EXIT_FAILURE;
}
```

---
## Пример работы приложений

<!-- #### Сообщение с подсказкой по использованию при неверно введённых аргументах при запуске сервера:

![serv_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_hw11/images/img1.png?raw=true)

#### Сообщение с подсказкой по использованию при неверно введённых аргументах при запуске клиента (у обоих клиентов подсказки одинаковые):

![client_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_hw11/images/img2.png?raw=true)

#### Запуск сервера

![client_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_hw11/images/img3.png?raw=true)

#### Запуск одного из клиентов, сервер сообщил информацию о новом клиенте

![client_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_hw11/images/img4.png?raw=true)

#### Запуск второго клиента, сервер сообщил информацию о новом клиенте

![client_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_hw11/images/img5.png?raw=true)

#### Первому клиенту было введено 4 сообщения, сервер переотправил их второму клиенту, который вывел их на экран

![client_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_hw11/images/img6.png?raw=true)

#### Завершение работы всех трёх программ при вводе сообщения "The End"

![client_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_hw11/images/img7.png?raw=true) -->

---

Для компиляции программ можно запустить скрипт:

    ./compile.sh
