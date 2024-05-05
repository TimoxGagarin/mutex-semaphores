#pragma once
#include <stdint.h>
#define MAX_LEN_RECORD 80
#define COUNT_RECORDS 10

typedef struct record_t
{
    char name[MAX_LEN_RECORD];
    char address[MAX_LEN_RECORD];
    uint8_t semester;
} record_t;