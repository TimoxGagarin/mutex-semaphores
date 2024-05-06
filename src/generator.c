#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "headers/students.h"

int main()
{
    FILE *file = fopen("file.bin", "wb");
    if (file == NULL)
    {
        perror("Failure opening file");
        exit(EXIT_FAILURE);
    }

    record_t records[COUNT_RECORDS];
    for (int i = 0; i < COUNT_RECORDS; ++i)
    {
        sprintf(records[i].name, "Name %d", i);
        sprintf(records[i].address, "Address %d", i);
        records[i].semester = i % 5 + 1;
    }

    fwrite(records, sizeof(record_t), COUNT_RECORDS, file);
    fclose(file);
    puts("Data recording was successful.");
    exit(EXIT_SUCCESS);
}