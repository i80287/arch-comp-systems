кормилицын владимир алексеевич, БПИ226

## Описание программ

**Для выполнения задачи были написаны 3 программы:**
* #### server.c - сервер, получающий сообщения от первого клиенты и перенаправляющий их второму клиенту
* #### first_client.c - клиент, считвающий текст из стандартного потока ввода и отправляющий их серверу
* #### second_client.c - клиента, принимающий сообщения от сервера и выволящий их в стандартный поток вывода

#### Сервер парсит входные аргументы, и, если они введены корректно, создаёт и настраивает сокет (в функции `create_server_socket`) и ожидает 2-х клиентов (функция `accept_client` вызывается 2 раза)

```c
int main(int argc, const char* argv[]) {
    uint16_t server_port;
    if (argc != 2 || !parse_port(argv[1], &server_port)) {
        report_invalid_command_args(argv[0]);
        return EXIT_FAILURE;
    }

    return run_server(server_port);
}
```

#### Функция для создания и настройки сокета:
```c
static inline int create_server_socket(uint16_t server_port) {
    int server_sock_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock_fd == -1) {
        perror("socket");
        return -1;
    }
    const struct sockaddr_in server_socket_address = {
        .sin_family      = AF_INET,
        .sin_port        = htons(server_port),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    bool bind_failed = bind(server_sock_fd, (const struct sockaddr*)&server_socket_address,
                            sizeof(server_socket_address)) == -1;
    if (bind_failed) {
        perror("bind");
        goto create_server_socket_cleanup;
    }
    bool listen_failed = listen(server_sock_fd, MAX_NUMBER_OF_CLIENTS) == -1;
    if (listen_failed) {
        perror("listen");
        goto create_server_socket_cleanup;
    }
    print_server_info((const struct sockaddr*)&server_socket_address,
                      sizeof(server_socket_address));
    return server_sock_fd;
create_server_socket_cleanup:
    close(server_sock_fd);
    return -1;
}
```

#### Функция для ожидания и принятия клиента:
```c
static inline int accept_client(int server_sock_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    int client_sock_fd =
        accept(server_sock_fd, (struct sockaddr*)&client_addr, &client_addrlen);
    if (client_sock_fd == -1) {
        perror("accept");
    } else {
        print_client_info((const struct sockaddr*)&client_addr, client_addrlen);
    }
    return client_sock_fd;
}
```

#### Клиенты парсят входные аргументы и, если они введены корректно, создают сокеты и подключаются к серверу
```c
int main(int argc, const char* argv[]) {
    ParseResult res = parse_args(argc, argv);
    if (!res.succeeded) {
        report_invalid_command_args(argv[0]);
        return EXIT_FAILURE;
    }

    return run_client(res.server_ip, res.port);
}
```

#### Функция для создания сокета:
```c
static inline int create_client_socket() {
    int sock_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_fd < 0) {
        perror("socket");
    }
    return sock_fd;
}
```

#### Функция для подключения к серверу:
```c
static inline bool connect_to_server(int sock_fd, const char* server_ip,
                                     uint16_t server_port) {
    const struct sockaddr_in server_socket_address = {
        .sin_family      = AF_INET,
        .sin_port        = htons(server_port),
        .sin_addr.s_addr = inet_addr(server_ip),
    };

    bool failed = connect(sock_fd, (const struct sockaddr*)&server_socket_address,
                          sizeof(server_socket_address)) == -1;
    if (failed) {
        perror("connect");
    }
    return !failed;
}
```

#### Алгоритм работы клиента, посылающего прочитанные сообщения серверу:
```c
static int run_client_impl(int sock_fd, const char* server_ip, uint16_t server_port) {
    if (!connect_to_server(sock_fd, server_ip, server_port)) {
        return EXIT_FAILURE;
    }

    char message_buffer[MESSAGE_BUFFER_SIZE] = {0};
    bool received_end_message                = false;
    while (!received_end_message &&
           fgets(message_buffer, MESSAGE_BUFFER_SIZE, stdin) != NULL) {
        size_t input_length = strlen(message_buffer);
        if (input_length == 0) {
            continue;
        }
        if (message_buffer[input_length - 1] == '\n') {
            message_buffer[input_length - 1] = '\0';
            input_length--;
        }

        const size_t send_length = input_length + 1;
        if (send(sock_fd, message_buffer, send_length, 0) != (ssize_t)send_length) {
            perror("send");
            break;
        }

        printf("Sent message of size %zu to the server\n", input_length);
        received_end_message = input_length == END_MESSAGE_LENGTH &&
                               memcmp(message_buffer, END_MESSAGE, END_MESSAGE_LENGTH) == 0;
    }

    printf("Client is stopping...");
    return received_end_message ? EXIT_SUCCESS : EXIT_FAILURE;
}
```

#### Алгоритм работы клиента, печатающего сообщения, полученные от сервера:
```c
static int run_client_impl(int sock_fd, const char* server_ip, uint16_t server_port) {
    int res = EXIT_FAILURE;
    if (!connect_to_server(sock_fd, server_ip, server_port)) {
        return res;
    }

    char message_buffer[MESSAGE_BUFFER_SIZE] = {0};
    bool received_end_message                = false;
    do {
        const ssize_t received_size = recv(sock_fd, message_buffer, MESSAGE_BUFFER_SIZE, 0);
        if (received_size <= 0) {
            perror("recv");
            break;
        }

        const size_t message_string_length = (size_t)received_size - 1;
        received_end_message = message_string_length == END_MESSAGE_LENGTH &&
                               memcmp(message_buffer, END_MESSAGE, END_MESSAGE_LENGTH) == 0;
        printf("Received message '%s' of length %zu from the server\n", message_buffer,
               message_string_length);
    } while (!received_end_message);

    printf("Client is stopping...");
    return received_end_message ? EXIT_SUCCESS : EXIT_FAILURE;
}
```

#### Алгоритм работы сервера, пересылающего сообщения от первого клиента ко второму:
```c
static int run_server_impl(int first_client_sock_fd, int second_client_sock_fd) {
    char message_buffer[MESSAGE_BUFFER_SIZE] = {0};
    bool received_end_message                = false;
    do {
        const ssize_t received_size = recv(first_client_sock_fd, message_buffer, MESSAGE_BUFFER_SIZE, 0);
        if (received_size <= 0) {
            perror("recv");
            break;
        }

        received_end_message = received_size == END_MESSAGE_LENGTH &&
                               memcmp(message_buffer, END_MESSAGE, END_MESSAGE_LENGTH) == 0;
        const size_t send_size = (size_t)received_size;
        printf(
            "Transfering message '%s' of length %zu from the first client to the second one\n",
            message_buffer, send_size);
        if (send(second_client_sock_fd, message_buffer, send_size, 0) != (ssize_t)send_size) {
            perror("send");
            break;
        }
    } while (!received_end_message);

    printf("Server is stopping...");
    return received_end_message ? EXIT_SUCCESS : EXIT_FAILURE;
}
```
---
## Пример работы приложений

#### Сообщение с подсказкой по использованию при неверно введённых аргументах при запуске сервера:

![serv_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_hw10/images/img1.png?raw=true)

#### Сообщение с подсказкой по использованию при неверно введённых аргументах при запуске клиента (у обоих клиентов подсказки одинаковые):

![client_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_hw10/images/img2.png?raw=true)

#### Запуск сервера

![client_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_hw10/images/img3.png?raw=true)

#### Запуск одного из клиентов, сервер сообщил информацию о новом клиенте

![client_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_hw10/images/img4.png?raw=true)

#### Запуск второго клиента, сервер сообщил информацию о новом клиенте

![client_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_hw10/images/img5.png?raw=true)

#### Первому клиенту было введено 4 сообщения, сервер переотправил их второму клиенту, который вывел их на экран

![client_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_hw10/images/img6.png?raw=true)

#### Завершение работы всех трёх программ при вводе сообщения "The End"

![client_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_hw10/images/img7.png?raw=true)

---

Для компиляции программ можно запустить скрипт:

    ./compile.sh
