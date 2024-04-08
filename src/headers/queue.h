#pragma once
#include "constants.h"
#include "msg.h"

typedef struct
{
    int added_amount;
    int extracted_amount;
    int max_size;

    int msg_count;

    int head;
    int tail;
    msg_t buffer[MSG_MAX];
} queue_t;

void new_queue(queue_t *);
int push(queue_t *, msg_t *);
int pop(queue_t *, msg_t *);