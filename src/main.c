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

pthread_cond_t cond_producer = PTHREAD_COND_INITIALIZER;
pthread_t producers[THREADS_MAX];
int producers_amount;

pthread_cond_t cond_consumer = PTHREAD_COND_INITIALIZER;
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
    queue = (queue_t *)malloc(sizeof(queue_t));
    new_queue(queue);
    if (pthread_mutex_init(&mutex, NULL))
    {
        perror("Failed mutex init");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_init(&cond_producer, NULL))
    {
        perror("Error initializing cond_producer");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_init(&cond_consumer, NULL))
    {
        perror("Error initializing cond_consumer");
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
    for (int i = producers_amount; i >= 0; i--)
    {
        if (pthread_cancel(producers[i]))
        {
            perror("Failed to cancel producer");
            exit(EXIT_FAILURE);
        }
        if (pthread_join(producers[i], NULL))
        {
            perror("Failed to join producer");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = consumers_amount; i >= 0; i--)
    {
        if (pthread_cancel(consumers[i]))
        {
            perror("Failed to cancel consumer");
            exit(EXIT_FAILURE);
        }
        if (pthread_join(consumers[i], NULL))
        {
            perror("Failed to join consumer");
            exit(EXIT_FAILURE);
        }
    }

    if (pthread_mutex_destroy(&mutex))
    {
        perror("Failed to destroy mutex");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_destroy(&cond_producer))
    {
        perror("Failed to destroy cond_producer");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_destroy(&cond_consumer))
    {
        perror("Failed to destroy cond_consumer");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
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
        new_msg(&msg);
        pthread_mutex_lock(&mutex);
        while (queue->msg_count == MSG_MAX - 1)
            pthread_cond_wait(&cond_producer, &mutex);
        add_count_local = push(queue, &msg);
        pthread_cond_signal(&cond_consumer);
        pthread_mutex_unlock(&mutex);
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
        pthread_mutex_lock(&mutex);
        while (queue->msg_count == 0)
            pthread_cond_wait(&cond_consumer, &mutex);
        extract_count_local = pop(queue, &msg);
        pthread_cond_signal(&cond_producer);
        pthread_mutex_unlock(&mutex);
        handle_msg(&msg);
        printf("%ld consume msg_t: hash=%X, extracted_amount=%d\n",
               pthread_self(), msg.hash, extract_count_local);
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