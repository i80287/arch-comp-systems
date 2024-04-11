// Заголовочный файл, содержащий общие данные для писателей и читателей
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>

enum { BUF_SIZE = 10 }; // Размер буфера ячеек
enum { MAX_READERS = 2 };

// Структура для хранения в разделяемой памяти буфера и необходимых данных
// Задает буфер, доступный процессам
typedef struct {
    int store[BUF_SIZE];  // буфер для заполнения ячеек
    volatile size_t write_index;
    volatile size_t read_index;
    pid_t reader_pids[MAX_READERS];  // идентификаторы процессов читателей
    size_t size_of_reader_pids;    // количество читателей
    pid_t writer_pid;  // идентификатор процесса писателя
} shared_memory;

// имя области разделяемой памяти
extern const char *shar_object;
extern int buf_id;  // дескриптор объекта памяти
extern shared_memory *buffer;  // указатель на разделямую память, хранящую буфер

// имя семафора для занятых ячеек
extern const char *full_sem_name;
extern sem_t *full;  // указатель на семафор занятых ячеек

// имя семафора для свободных ячеек
extern const char *empty_sem_name;
extern sem_t *empty;  // указатель на семафор свободных ячеек

// имя семафора (мьютекса) для чтения данных из буфера
extern const char* read_mutex_name;
extern sem_t *read_mutex;  // указатель на семафор читателей

// имя семафора (мьютекса) для записи данных в буфер
extern const char* write_mutex_name;
extern sem_t *write_mutex;  // указатель на семафор писаталей

// Имя семафора для управления доступом.
// Позволяет читателю дождаться разрешения от читателя.
// Даже если читатель стартовал первым
extern const char *admin_sem_name;
extern sem_t *admin;  // указатель на семафор читателей

// Функция осуществляющая при запуске общие манипуляции с памятью и
// семафорами для децентрализованно подключаемых процессов читателей и
// писателей.
void init(void);

// Функция закрывающая семафоры общие для писателей и читателей
void close_common_semaphores(void);

// Функция, удаляющая все семафоры и разделяемую память
void unlink_all(void);
