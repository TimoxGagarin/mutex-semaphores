#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include "headers/students.h"

int descriptor = 0;
bool FLAG_EDIT = false;

bool is_equal(const record_t *a, const record_t *b);
void reccpy(record_t *to, const record_t *from);
void read_all();
bool get_i_record(size_t i, record_t *record_);
void update_n_records(size_t n, record_t *record_, record_t *savedRecord);
bool add_i_record(record_t *newRecord, const record_t *record_, const record_t *savedRecord, size_t i);

int main()
{
    bool cycleFlag = true;
    record_t REC;
    record_t SAVED_REC;
    record_t NEW_REC;
    size_t REC_NUM = 0;

    descriptor = open("file.bin", O_RDWR);
    if (descriptor == -1)
    {
        puts("ERROR: Wrong file name.");
        exit(EXIT_FAILURE);
    }

    char ch = 0;
    do
    {
        if (ch != '\n')
            puts("1.Display all records.\n2.Update record_t.\n3.Get num_record.\n4.Add last modified record_t.\n5.Exit.");
        ch = getchar();
        if (ch == '\n')
            continue;
        else if (ch == '1')
            read_all();
        else if (ch == '2')
        {
            puts("Enter the num of record_t: ");
            scanf("%lu", &REC_NUM);
            update_n_records(REC_NUM, &REC, &SAVED_REC);
        }
        else if (ch == '3')
        {
            record_t record_t;
            size_t num;
            puts("Enter the num of record_t: ");
            scanf("%lu", &num);
            if (!get_i_record(num, &record_t))
            {
                printf("Cannot find %i record_t\n", num);
                continue;
            }
            printf("%lu. Name: %s, Address: %s, Semester: %hhu\n", num, record_t.name, record_t.address, record_t.semester);
        }
        else if (ch == '4')
        {
            if (!FLAG_EDIT)
                puts("No entry has been changed!");
            else if (!add_i_record(&NEW_REC, &REC, &SAVED_REC, REC_NUM))
            {
                puts("The data has been changed by someone, please repeat the editing operation again.");
                update_n_records(REC_NUM, &REC, &SAVED_REC);
            }
        }
        else if (ch == '5')
            cycleFlag = false;
        else
            puts("Incorrect input.");
    } while (cycleFlag);

    close(descriptor);
    exit(EXIT_FAILURE);
}

bool is_equal(const record_t *a, const record_t *b)
{
    if (!a || !b)
    {
        puts("Empty records to compare");
        exit(EXIT_FAILURE);
    }
    return strcmp(a->name, b->name) == 0 && strcmp(a->address, b->address) == 0 && a->semester == b->semester;
}

void reccpy(record_t *to, const record_t *from)
{
    if (!to || !from)
    {
        puts("Empty records to copy");
        exit(EXIT_FAILURE);
    }
    strncpy(to->name, from->name, MAX_LEN_RECORD);
    strncpy(to->address, from->address, MAX_LEN_RECORD);
    to->semester = from->semester;
}

void read_all()
{
    struct flock parameters;
    parameters.l_type = F_RDLCK;
    parameters.l_whence = SEEK_SET;
    parameters.l_start = 0;
    parameters.l_len = 0;
    if (fcntl(descriptor, F_SETLKW, &parameters) < 0)
        perror("F_SETLKW");
    record_t buffer;
    for (size_t i = 0; i < COUNT_RECORDS; ++i)
    {
        read(descriptor, &buffer, sizeof(record_t));
        printf("%lu. Name: %s, Address: %s, Semester: %hhu\n", i + 1, buffer.name, buffer.address, buffer.semester);
    }
    parameters.l_type = F_UNLCK;
    if (fcntl(descriptor, F_SETLKW, &parameters) < 0)
        perror("F_SETLKW");
    lseek(descriptor, 0, SEEK_SET);
}

bool get_i_record(size_t i, record_t *record_)
{
    if (i > COUNT_RECORDS || i == 0)
        return false;
    struct flock parameters;
    parameters.l_type = F_RDLCK;
    parameters.l_whence = SEEK_SET;
    parameters.l_start = (i - 1) * sizeof(record_t);
    parameters.l_len = sizeof(record_t);
    if (fcntl(descriptor, F_SETLKW, &parameters) < 0)
        perror("F_SETLKW");
    lseek(descriptor, (i - 1) * sizeof(record_t), SEEK_SET);
    read(descriptor, record_, sizeof(record_t));
    parameters.l_type = F_UNLCK;
    if (fcntl(descriptor, F_SETLKW, &parameters) < 0)
        perror("F_SETLKW");
    lseek(descriptor, 0, SEEK_SET);
    return true;
}

void update_n_records(size_t n, record_t *record_, record_t *savedRecord)
{
    if (!get_i_record(n, record_))
    {
        puts("No record_t founded");
        return;
    }
    reccpy(savedRecord, record_);
    bool continueFlag = true;
    char ch = 0;
    do
    {
        if (ch != '\n')
            puts("1.Update name;\n2.Update address;\n3.Update semester;\n4.Display current record_t;\n5.Main menu.");
        ch = getchar();
        char buffer[MAX_LEN_RECORD];
        if (ch == '\n')
            continue;
        else if (ch == '1')
        {
            FLAG_EDIT = true;
            printf("Enter the name: ");
            scanf("%s", buffer);
            strncpy(record_->name, buffer, MAX_LEN_RECORD);
        }
        else if (ch == '2')
        {
            FLAG_EDIT = true;
            printf("Enter the address: ");
            scanf("%s", buffer);
            strncpy(record_->address, buffer, MAX_LEN_RECORD);
        }
        else if (ch == '3')
        {
            FLAG_EDIT = true;
            u_int8_t sem;
            puts("Enter the num of semester: ");
            scanf("%hhu", &sem);
            record_->semester = sem;
        }
        else if (ch == '4')
            printf("%lu. Name: %s, Address: %s, Num of semester: %hhu\n",
                   n, record_->name, record_->address, record_->semester);
        else if (ch == '5')
            continueFlag = false;
        else if (ch == '\n')
            continue;
        else
            puts("Incorrect input.");
    } while (continueFlag);
    FLAG_EDIT = !is_equal(record_, savedRecord);
}

bool add_i_record(record_t *newRecord, const record_t *record_, const record_t *savedRecord, size_t i)
{
    struct flock parameters;
    parameters.l_type = F_WRLCK;
    parameters.l_whence = SEEK_SET;
    parameters.l_start = (i - 1) * sizeof(record_t);
    parameters.l_len = sizeof(record_t);
    if (fcntl(descriptor, F_SETLKW, &parameters) < 0)
        perror("F_SETLKW");
    parameters.l_type = F_UNLCK;
    get_i_record(i, newRecord);
    if (!is_equal(savedRecord, newRecord))
    {
        fcntl(descriptor, F_SETLK, &parameters);
        return false;
    }
    lseek(descriptor, (i - 1) * sizeof(record_t), SEEK_SET);
    write(descriptor, record_, sizeof(record_t));
    if (fcntl(descriptor, F_SETLKW, &parameters) < 0)
        perror("F_SETLKW");
    lseek(descriptor, 0, SEEK_SET);
    FLAG_EDIT = false;
    return true;
}