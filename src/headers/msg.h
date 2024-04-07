#pragma once
#include "constants.h"

typedef struct
{
    int type;
    int hash;
    int size;
    char data[DATA_MAX];
} msg_t;

int hash(msg_t *);
void new_msg(msg_t *);
void handle_msg(msg_t *);