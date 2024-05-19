# Отчёт о выполении ДЗ №11

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
    int setsockopt_res             = setsockopt(
        server_sock_fd, SOL_SOCKET, SO_BROADCAST,
        (const void*)&broadcast_permission, sizeof(broadcast_permission));

    if (setsockopt_res == -1) {
        perror("setsockopt");
        close(server_sock_fd);
        return -1;
    }

    return server_sock_fd;
}
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
static inline int create_client_socket(uint16_t client_port) {
    int sock_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd == -1) {
        perror("socket");
        return -1;
    }

    const struct sockaddr_in broadcast_address = {
        .sin_family      = AF_INET,
        .sin_port        = htons(client_port),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    bool bind_failed =
        bind(sock_fd, (const struct sockaddr*)&broadcast_address,
             sizeof(broadcast_address)) == -1;
    if (bind_failed) {
        perror("bind");
        close(sock_fd);
        return -1;
    }

    printf("Client ready to read messages from the address:\n");
    print_socket_address_info(
        (const struct sockaddr_storage*)&broadcast_address,
        sizeof(broadcast_address));

    return sock_fd;
}
```

#### Алгоритм работы сервера, посылающего считанные сообщения клиентам:
```c
static int run_server_impl(int server_sock_fd,
                           const struct sockaddr_in broadcast_address) {
    char message_buffer[MESSAGE_BUFFER_SIZE] = {0};
    bool received_end_message                = false;
    printf("Ready to send messages to the address:\n");
    print_socket_address_info(
        (const struct sockaddr_storage*)&broadcast_address,
        sizeof(broadcast_address));
    do {
        printf("Enter message:\n> ");
        if (fgets(message_buffer, MESSAGE_BUFFER_SIZE, stdin) == NULL) {
            break;
        }

        size_t message_size = strlen(message_buffer);
        if (message_buffer[message_size - 1] == '\n') {
            message_buffer[message_size - 1] = '\0';
            message_size--;
        }

        received_end_message =
            is_end_message(message_buffer, message_size);
        size_t send_size = message_size + 1;
        if (sendto(server_sock_fd, message_buffer, send_size, 0,
                   (const struct sockaddr*)&broadcast_address,
                   sizeof(broadcast_address)) != (ssize_t)send_size) {
            perror("send");
            break;
        }

        printf(
            "Sent message \"%s\" of length %zu to all listeners\n",
            message_buffer, message_size);
    } while (!received_end_message);

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
        struct sockaddr_storage broadcast_address_storage;
        socklen_t broadcast_address_size =
            sizeof(broadcast_address_storage);
        ssize_t received_size =
            recvfrom(sock_fd, message_buffer, MESSAGE_BUFFER_SIZE, 0,
                     (struct sockaddr*)&broadcast_address_storage,
                     &broadcast_address_size);

        if (received_size < 0) {
            perror("recvfrom");
            break;
        }

        const size_t message_string_length    = (size_t)received_size - 1;
        message_buffer[message_string_length] = '\0';
        received_end_message =
            is_end_message(message_buffer, message_string_length);

        print_received_message_with_info(
            message_buffer, message_string_length,
            &broadcast_address_storage, broadcast_address_size);
    } while (!received_end_message);

    printf("Client is stopping...\n");
    return received_end_message ? EXIT_SUCCESS : EXIT_FAILURE;
}
```

#### Функция для вывода инфорации об адресе сокета:
```c
static inline void print_socket_address_info(
    const struct sockaddr_storage* socket_address_storage,
    socklen_t socket_address_size) {
    const struct sockaddr* socket_address =
        (const struct sockaddr*)socket_address_storage;
    char host_name[1024] = {0};
    char port_str[16]    = {0};
    int gai_err =
        getnameinfo(socket_address, socket_address_size, host_name,
                    sizeof(host_name), port_str, sizeof(port_str),
                    NI_NUMERICHOST | NI_NUMERICSERV | NI_DGRAM);
    if (gai_err == 0) {
        printf(
            "Socket address: %s\n"
            "Port: %s\n",
            host_name, port_str);
    } else {
        fprintf(stderr,
                "Could not fetch info about current socket address: %s\n",
                gai_strerror(gai_err));
    }

    gai_err = getnameinfo(socket_address, socket_address_size, host_name,
                          sizeof(host_name), port_str, sizeof(port_str),
                          NI_DGRAM);
    if (gai_err == 0) {
        printf(
            "Readable socket address form: %s\nReadable port form: %s\n",
            host_name, port_str);
    }
}
```

---
## Пример работы приложений

#### Запуск сервера на компьютере, находящемся по адресу 192.168.1.9 в локальной сети. Сервер готов посылать сообщения на адрес 192.168.1.3:49524

![serv_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_hw11/images/1.png?raw=true)

#### Запуск клиента на компьютере, находящемся по адресу 192.168.1.3 (для клиента - 0.0.0.0) в локальной сети. Клиент готов слушать на порте 49524

![client_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_hw11/images/2.png?raw=true)

#### Было введено 2 сообщения, которые сервер отправил всем слушателям по адресу 192.168.1.3:49524

![client_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_hw11/images/3.png?raw=true)

#### Клиент получил 2 сообщения от сервера, отправленных с адреса 192.168.1.9:34003

![client_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_hw11/images/4.png?raw=true)

#### Было введено сообщение "The End" о завершении работы, которое было отправлено всем слушателям по адресу 192.168.1.3:49524

![client_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_hw11/images/5.png?raw=true)

#### Клиент получил сообщение "The End" и завершил работу

![client_invalid_args_hint](https://github.com/i80287/arch-comp-systems/blob/main/os_hw11/images/6.png?raw=true)

---

Для компиляции программ можно запустить скрипт:

    ./compile.sh
