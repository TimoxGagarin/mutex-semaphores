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
#include <pthread.h>

#include "headers/constants.h"
#include "headers/msg.h"
#include "headers/queue.h"

queue_t *queue;
pthread_mutex_t mutex;

sem_t *free_space;
sem_t *items;

pthread_t producers[THREADS_MAX];
int producers_amount;

pthread_t consumers[THREADS_MAX];
int consumers_amount;

/**
 * @brief Создание нового процесса.
 *
 * Функция создает новый поток, используя указанную функцию.
 *
 * @param list Массив идентификаторов потоков.
 * @param count Указатель на переменную, хранящую количество созданных потоков.
 * @param func Указатель на функцию, которая будет выполнена в созданном потоке.
 */
void new_process(pthread_t *list, int *count, void *(*func)(void *))
{
    // Проверка, достигнуто ли максимальное количество потоков.
    if (*count == THREADS_MAX - 1)
    {
        fprintf(stderr, "Max value of threads\n");
        return;
    }
    // Создание нового потока.
    if (pthread_create(&list[*count], NULL, func, NULL))
    {
        perror("Failed to create thread\n");
        exit(EXIT_FAILURE);
    }
    ++(*count);
}

/**
 * @brief Закрытие потока.
 *
 * Функция закрывает поток, останавливая его выполнение и освобождая ресурсы.
 *
 * @param list Массив идентификаторов потоков.
 * @param count Указатель на переменную, хранящую количество потоков в массиве.
 */
void close_process(pthread_t *list, int *count)
{
    // Проверка, есть ли потоки для закрытия.
    if (*count == 0)
    {
        fprintf(stderr, "No thread to delete\n");
        return;
    }

    // Уменьшение счетчика потоков и ожидание завершения потока.
    (*count)--;
    pthread_cancel(list[*count]);
    pthread_join(list[*count], NULL);
}

/**
 * @brief Инициализация.
 *
 * Функция инициализирует необходимые ресурсы для работы программы.
 */
void init(void)
{
    // Выделение памяти под очередь и инициализация очереди.
    queue = (queue_t *)malloc(sizeof(queue_t));
    new_queue(queue);

    // Инициализация мьютекса.
    int res = pthread_mutex_init(&mutex, NULL);
    if (res)
    {
        perror("Failed mutex init");
        exit(EXIT_FAILURE);
    }

    // Создание семафоров.
    if ((free_space = sem_open("free_space", (O_RDWR | O_CREAT | O_TRUNC), (S_IRUSR | S_IWUSR), START_MAX)) == SEM_FAILED ||
        (items = sem_open("items", (O_RDWR | O_CREAT | O_TRUNC), (S_IRUSR | S_IWUSR), 0)) == SEM_FAILED)
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
    // Уничтожение мьютекса.
    int res = pthread_mutex_destroy(&mutex);
    if (res)
    {
        perror("Failed mutex destroy");
        exit(EXIT_FAILURE);
    }
    // Удаление семафоров.
    if (sem_unlink("free_space") ||
        sem_unlink("items"))
    {
        perror("sem_unlink");
        abort();
    }
}

/**
 * @brief Процесс производителя.
 *
 * Функция, выполняемая процессом производителя.
 */
void *producer_process(void *arg)
{
    msg_t msg;
    int add_count_local;
    while (true)
    {
        if (queue->max_size == 0)
        {
            fprintf(stderr, "Queue size is 0 \n");
            exit(EXIT_FAILURE);
        }
        new_msg(&msg);
        sem_wait(free_space);
        pthread_mutex_lock(&mutex);

        add_count_local = push(queue, &msg);
        pthread_mutex_unlock(&mutex);
        sem_post(items);

        printf("%ld produce msg_t: hash=%X, added_amount=%d\n",
               pthread_self(), msg.hash, add_count_local);
        sleep(4);
    }
}

/**
 * @brief Процесс потребителя.
 *
 * Функция, выполняемая процессом потребителя.
 */
void *consumer_process(void *args)
{
    msg_t msg;
    int extract_count_local;
    while (true)
    {
        if (queue->max_size == 0)
        {
            fprintf(stderr, "Queue size is 0 \n");
            exit(EXIT_FAILURE);
        }
        sem_wait(items);
        pthread_mutex_lock(&mutex);

        extract_count_local = pop(queue, &msg);

        pthread_mutex_unlock(&mutex);
        sem_post(free_space);
        handle_msg(&msg);

        printf("%ld consume msg_t: hash=%X, extracted_amount=%d\n",
               pthread_self(), msg.hash, extract_count_local);
        sleep(4);
    }
}

/**
 * @brief Увеличение максимального размера очереди.
 *
 * Функция увеличивает максимальный размер очереди на 1, если это возможно,
 * и увеличивает счетчик свободного места в очереди.
 */
void increase_max_size()
{
    pthread_mutex_lock(&mutex);
    if (queue->max_size != MSG_MAX)
    {
        queue->max_size++;
        sem_post(free_space);
    }
    pthread_mutex_unlock(&mutex);
}

/**
 * @brief Уменьшение максимального размера очереди.
 *
 * Функция уменьшает максимальный размер очереди на 1, если это возможно,
 * и обновляет счетчики свободного места и количества элементов в очереди.
 */
void decrease_max_size()
{
    pthread_mutex_lock(&mutex);
    if (queue->max_size != 0)
    {
        if (queue->msg_count != 0)
        {
            queue->tail--;
            if (queue->msg_count != queue->max_size)
                sem_wait(free_space);
            queue->msg_count--;
            sem_wait(items);
        }
        queue->max_size--;
    }

    printf("%ld consume msg_t:extracted_amount=%d\n",
           pthread_self(), queue->msg_count);
    pthread_mutex_unlock(&mutex);
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
           "+: increase size by 1\n"
           "-: decrease size by 1\n"
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
        else if (operation == '+')
            increase_max_size();
        else if (operation == '-')
            decrease_max_size();
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