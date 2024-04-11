// reader.c - читатель забирающий из первой по очереди занятой ячейки
#include <unistd.h>

#include "common.h"
#include <assert.h>
// Семафор для отсекания лишних читателей
// То есть, для формирования только одного процесса-читателя
const char *reader_sem_name = "/reader-semaphore";
sem_t *reader;  // указатель на семафор пропуска читателей

void sigfunc(int sig) {
    if (sig != SIGINT && sig != SIGTERM) {
        return;
    }
    pid_t this_pid = getpid();
    if (sig == SIGINT) {
        pid_t writer_pid = buffer->writer_pid;
        kill(writer_pid, SIGTERM);
        printf("Reader[pid = %d](SIGINT) ---> Writer[pid = %d](SIGTERM)\n", this_pid, writer_pid);
        for (size_t i = 0; i < buffer->size_of_reader_pids; i++) {
            pid_t another_reader_pid = buffer->reader_pids[i];
            if (another_reader_pid == this_pid) {
                continue;
            }
            kill(another_reader_pid, SIGTERM);
            printf("Reader[pid = %d](SIGINT) ---> Reader[pid = %d](SIGTERM)\n", this_pid, another_reader_pid);
        }
    } else if (sig == SIGTERM) {
        printf("Reader[pid = %d](SIGTERM) <--- Writer/Reader(SIGINT)\n", this_pid);
    }
    // Закрывает свой семафор
    if (sem_close(reader) == -1) {
        perror("sem_close: Incorrect close of reader semaphore");
        exit(-1);
    }
    // Удаляет свой семафор
    sem_unlink(reader_sem_name);
    printf("Reader[pid = %d]: bye!!!\n", this_pid);
    exit(10);
}

// Функция вычисления факториала
int factorial(int n) {
    int p = 1;
    for (int i = 1; i <= n; ++i) {
        p *= i;
    }
    return p;
}

int main() {
    signal(SIGINT, sigfunc);
    signal(SIGTERM, sigfunc);

    srand((unsigned)time(0));  // Инициализация генератора случайных чисел
    init();  // Начальная инициализация общих семафоров
    // printf("init checkout\n");

    // Если читатель запустился раньше, он ждет разрешения от
    // администратора, Который находится в писателе
    printf("Consumer %d started waiting for the producer\n", getpid());
    if (sem_wait(admin) == -1) {  // ожидание когда запустится писатель
        perror("sem_wait: Incorrect wait of admin semaphore");
        exit(-1);
    };
    printf("Consumer %d started\n", getpid());
    fflush(stdout);
    // После этого он вновь поднимает семафор, позволяющий запустить других
    // читателей
    if (sem_post(admin) == -1) {
        perror("sem_post: Consumer can not increment admin semaphore");
        exit(-1);
    }

    // Читатель имеет доступ только к открытому объекту памяти,
    // но может не только читать, но и писать (забирать)
    if ((buf_id = shm_open(shar_object, O_RDWR, 0666)) == -1) {
        perror("shm_open: Incorrect reader access");
        exit(-1);
    } else {
        printf("Memory object is opened: name = %s, id = 0x%x\n",
               shar_object, buf_id);
        fflush(stdout);
    }
    // Задание размера объекта памяти
    if (ftruncate(buf_id, sizeof(shared_memory)) == -1) {
        perror("ftruncate");
        exit(-1);
    } else {
        printf("Memory size set and = %lu\n", sizeof(shared_memory));
        fflush(stdout);
    }

    // получить доступ к памяти
    buffer = mmap(0, sizeof(shared_memory), PROT_WRITE | PROT_READ,
                  MAP_SHARED, buf_id, 0);
    if (buffer == (shared_memory *)-1) {
        perror("reader: mmap");
        exit(-1);
    }

    // Разборки читателей. Семафор для конкуренции за работу
    if ((reader = sem_open(reader_sem_name, O_CREAT, 0666, 1)) == 0) {
        perror("sem_open: Can not create reader semaphore");
        exit(-1);
    };
    // Первый просочившийся запрещает доступ остальным за счет установки
    // флага
    if (sem_wait(reader) == -1) {  // ожидание когда запустится писатель
        perror("sem_wait: Incorrect wait of reader semaphore");
        exit(-1);
    };
    
    if (buffer->size_of_reader_pids >= MAX_READERS) {
        // Завершение процесса
        printf("Reader %d: I have lost this work :(\n", getpid());
        // Безработных читателей может быть много. Нужно их тоже
        // отфутболить. Для этого они должны войти в эту секцию
        if (sem_post(reader) == -1) {
            perror(
                "sem_post: Consumer can not increment reader semaphore");
            exit(-1);
        }
        exit(13);
    }
    // сохранение pid для корректного взаимодействия с писателем
    buffer->reader_pids[buffer->size_of_reader_pids++] = getpid();

    // Пропуск  читателей других для определения возможности поработать
    if (sem_post(reader) == -1) {
        perror("sem_post: Consumer can not increment reader semaphore");
        exit(-1);
    }

    assert(buffer->size_of_reader_pids <= MAX_READERS);

    // Алгоритм читателя
    while (1) {
        sleep((unsigned)rand() % 3 + 1);
        // Контроль наличия элементов в буфере
        if (sem_wait(full) == -1) {
            perror("sem_wait: Incorrect wait of full semaphore");
            exit(-1);
        };
        // критическая секция
        if (sem_wait(read_mutex) == -1) {
            perror("sem_wait: Incorrect wait of busy semaphore");
            exit(-1);
        };

        // Получение значения из читаемой ячейки
        size_t read_index = buffer->read_index;
        int result = buffer->store[read_index];
        assert(result >= 0);
        buffer->store[read_index] = -1;
        buffer->read_index = (read_index + 1) % BUF_SIZE;
        // вычисление факториала
        int f = factorial(result);

        // количество свободных ячеек увеличилось на единицу
        if (sem_post(empty) == -1) {
            perror("sem_post: Incorrect post of free semaphore");
            exit(-1);
        };
        // Вывод информации об операции чтения
        pid_t pid = getpid();
        printf(
            "Consumer %d: Reads value = %d from cell [%zu], factorial = "
            "%d\n",
            pid, result, read_index, f);
        fflush(stdout);
        // Выход из критической секции
        if (sem_post(read_mutex) == -1) {
            perror("sem_post: Incorrect post of mutex semaphore");
            exit(-1);
        };
    }
}
