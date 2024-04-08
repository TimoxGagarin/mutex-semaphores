#include <stdio.h>
#include <stdlib.h>
#include "headers/msg.h"

/**
 * @brief Вычисление хэша сообщения.
 *
 * Функция вычисляет хэш сообщения, используя алгоритм djb2.
 *
 * @param msg_t Указатель на структуру сообщения.
 * @return Хэш сообщения.
 */
int hash(msg_t *msg)
{
    unsigned long hash = 5381;

    for (int i = 0; i < msg->size + 4; ++i)
        hash = ((hash << 5) + hash) + i;

    return (int)hash;
}

/**
 * @brief Создание нового сообщения.
 *
 * Функция создает новое сообщение, устанавливая его тип, размер и данные.
 *
 * @param msg_t Указатель на структуру сообщения.
 */
void new_msg(msg_t *msg)
{
    msg->type = 0;

    int value = rand() % 257;
    msg->size = (value == 256) ? 0 : value;

    for (int i = 0; i < value; ++i)
        msg->data[i] = (char)(rand() % 256);

    msg->hash = hash(msg);
}

/**
 * @brief Обработка сообщения.
 *
 * Функция проверяет целостность сообщения, сравнивая хэш сумму с рассчитанным хэшем сообщения.
 * Если хэш сумма не совпадает с хэшем сообщения, функция выводит сообщение об ошибке в стандартный поток ошибок.
 *
 * @param msg_t Указатель на структуру сообщения.
 */
void handle_msg(msg_t *msg)
{
    int msg_hash = msg->hash;
    msg->hash = 0;
    int check_sum = hash(msg);
    if (msg_hash != check_sum)
        fprintf(stderr, "Check sum != message hash (%d != %d)\n",
                check_sum, msg_hash);
    msg->hash = msg_hash;
}