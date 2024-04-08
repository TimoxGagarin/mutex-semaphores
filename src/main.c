#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <inttypes.h>

#include "headers/constants.h"
#include "headers/msg.h"
#include "headers/queue.h"

queue_t *queue;
sem_t *mutex;

sem_t *free_space;
sem_t *items;

pid_t producers[CHILD_MAX];
int producers_amount;

pid_t consumers[CHILD_MAX];
int consumers_amount;

static pid_t parent_pid;

bool temp = false;

/**
 * @brief Обработчик сигнала.
 *
 * Функция-обработчик для сигнала.
 *
 * @param sig Номер сигнала.
 */
void handler(int sig)
{
    temp = true;
}

/**
 * @brief Создание нового процесса.
 *
 * Функция создает новый процесс, используя указанную функцию.
 *
 * @param list Массив идентификаторов процессов.
 * @param count Количество созданных процессов.
 * @param func Указатель на функцию, которая будет выполнена в созданном процессе.
 */
void new_process(pid_t *list, int *count, void (*func)(void))
{
    if (*count == CHILD_MAX - 1)
    {
        fprintf(stderr, "Max value of processes\n");
        return;
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        func();
    }
    else
    {
        list[*count] = pid;
        ++(*count);
        return;
    }
}

/**
 * @brief Закрытие процесса.
 *
 * Функция закрывает процесс, остановив его выполнение и освободив ресурсы.
 *
 * @param list Массив идентификаторов процессов.
 * @param count Количество процессов в массиве.
 */
void close_process(pid_t *list, int *count)
{
    if (*count == 0)
    {
        fprintf(stderr, "No process to delete\n");
        return;
    }

    (*count)--;
    kill(list[*count], SIGUSR1);
    wait(NULL);
}

/**
 * @brief Инициализация.
 *
 * Функция инициализирует необходимые ресурсы для работы программы.
 */
void init()
{
    parent_pid = getpid();

    int fd = shm_open("/queue", (O_RDWR | O_CREAT | O_TRUNC), (S_IRUSR | S_IWUSR));
    if (fd < 0)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, sizeof(queue_t)))
    {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    void *ptr = mmap(NULL, sizeof(queue_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    queue = (queue_t *)ptr;
    new_queue(queue);

    if (close(fd))
    {
        perror("close");
        exit(EXIT_FAILURE);
    }

    if ((mutex = sem_open("mutex", (O_RDWR | O_CREAT | O_TRUNC), (S_IRUSR | S_IWUSR), 1)) == SEM_FAILED || (free_space = sem_open("free_space", (O_RDWR | O_CREAT | O_TRUNC), (S_IRUSR | S_IWUSR), MSG_MAX)) == SEM_FAILED || (items = sem_open("items", (O_RDWR | O_CREAT | O_TRUNC), (S_IRUSR | S_IWUSR), 0)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Завершение работы программы.
 *
 * Функция завершает работу программы, освобождая выделенные ресурсы.
 */
void end()
{
    for (size_t i = 0; i < producers_amount; ++i)
    {
        kill(producers[i], SIGKILL);
        wait(NULL);
    }
    for (size_t i = 0; i < consumers_amount; ++i)
    {
        kill(consumers[i], SIGKILL);
        wait(NULL);
    }

    if (shm_unlink("/queue"))
    {
        perror("shm_unlink");
        abort();
    }
    if (sem_unlink("mutex") ||
        sem_unlink("free_space") ||
        sem_unlink("items"))
    {
        perror("sem_unlink");
        abort();
    }

    kill(parent_pid, SIGKILL);
}

/**
 * @brief Процесс производителя.
 *
 * Функция, выполняемая процессом производителя.
 */
void producer_process()
{
    srand(getpid());

    signal(SIGUSR1, handler);
    msg_t msg;
    int add_count_local;
    while (true)
    {
        new_msg(&msg);
        sem_wait(free_space);
        sem_wait(mutex);

        add_count_local = push(queue, &msg);
        sem_post(mutex);
        sem_post(items);

        printf("%d produce msg_t: hash=%X, added_amount=%d\n",
               getpid(), msg.hash, add_count_local);
        if (temp)
            exit(EXIT_SUCCESS);
        sleep(4);
    }
}

/**
 * @brief Процесс потребителя.
 *
 * Функция, выполняемая процессом потребителя.
 */
void consumer_process()
{
    signal(SIGUSR1, handler);
    msg_t msg;
    int extract_count_local;
    while (true)
    {
        sem_wait(items);
        sem_wait(mutex);

        extract_count_local = pop(queue, &msg);

        sem_post(mutex);
        sem_post(free_space);

        handle_msg(&msg);

        printf("%d consume msg_t: hash=%X, extracted_amount=%d\n",
               getpid(), msg.hash, extract_count_local);
        if (temp)
            exit(EXIT_SUCCESS);

        sleep(4);
    }
}

int main()
{
    init();

    signal(SIGKILL, end);
    signal(SIGTERM, end);
    signal(SIGINT, end);

    printf("Enter: \n"
           "p: new producer\n"
           "d: delete producer\n"
           "c: new consumer\n"
           "r: delete consumer\n"
           "q: exit\n");
    while (true)
    {
        char operation = getchar();
        if (operation == 'p')
            new_process(producers, &producers_amount, producer_process);
        else if (operation == 'd')
            close_process(producers, &producers_amount);
        else if (operation == 'c')
            new_process(consumers, &consumers_amount, consumer_process);
        else if (operation == 'r')
            close_process(consumers, &consumers_amount);
        else if (operation == 'q')
            break;
        else if (operation == '\n')
            continue;
        else
            printf("Unknown operator");
    }
    end();
    exit(EXIT_SUCCESS);
}