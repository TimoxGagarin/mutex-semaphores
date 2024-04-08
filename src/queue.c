#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "headers/queue.h"

/**
 * @brief Создание новой очереди.
 *
 * Функция инициализирует новую очередь, устанавливая начальные значения полей структуры.
 *
 * @param queue Указатель на структуру очереди.
 */
void new_queue(queue_t *queue)
{
    queue->added_amount = 0;
    queue->extracted_amount = 0;
    queue->msg_count = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->max_size = START_MAX;
    memset(queue->buffer, 0, sizeof(queue->buffer));
}

/**
 * @brief Добавление сообщения в очередь.
 *
 * Функция добавляет новое сообщение в очередь.
 *
 * @param queue Указатель на структуру очереди.
 * @param msg_t Указатель на структуру сообщения.
 * @return Количество успешно добавленных сообщений.
 */
int push(queue_t *queue, msg_t *msg_t)
{
    if (queue->msg_count == MSG_MAX - 1)
    {
        fprintf(stderr, "Queue buffer overflow\n");
        exit(EXIT_FAILURE);
    }

    if (queue->tail == MSG_MAX)
        queue->tail = 0;

    queue->buffer[queue->tail] = *msg_t;
    queue->tail++;
    queue->msg_count++;

    return ++queue->added_amount;
}

/**
 * @brief Извлечение сообщения из очереди.
 *
 * Функция извлекает сообщение из очереди.
 *
 * @param queue Указатель на структуру очереди.
 * @param msg_t Указатель на структуру сообщения, куда будет сохранено извлеченное сообщение.
 * @return Количество успешно извлеченных сообщений.
 */
int pop(queue_t *queue, msg_t *msg_t)
{
    if (queue->msg_count == 0)
    {
        fprintf(stderr, "Queue buffer underflow\n");
        exit(EXIT_FAILURE);
    }

    if (queue->head == MSG_MAX)
        queue->head = 0;

    *msg_t = queue->buffer[queue->head];
    queue->head++;
    queue->msg_count--;

    return ++queue->extracted_amount;
}