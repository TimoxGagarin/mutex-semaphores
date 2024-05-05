CC = gcc
CFLAGS = -g2 -ggdb -I./headers -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE -W -Wall -Wno-unused-parameter -Wno-unused-variable -std=c11 -pedantic

.SUFFIXES:
.SUFFIXES: .c .o

DEBUG = ./build/debug
RELEASE = ./build/release

OUT_DIR = $(DEBUG)
MSG_INFO = "Запуск производится в режиме debug..."
vpath %.c src
vpath %.h src
vpath %.o build/debug

ifeq ($(MODE), release)
	CFLAGS = -I./headers -W -Wall -Wno-unused-parameter -Wno-unused-variable -std=c11 -pedantic
	OUT_DIR = $(RELEASE)
	ENVS = CHILD_PATH=$(RELEASE)/child
	MSG_INFO = "Запуск производится в режиме release"
	vpath %.o build/release
endif

main_objects = $(OUT_DIR)/main.o $(OUT_DIR)/queue.o $(OUT_DIR)/msg.o
main_prog = $(OUT_DIR)/main

students_objects = $(OUT_DIR)/students.o
students_prog = $(OUT_DIR)/students

generator_objects = $(OUT_DIR)/generator.o
generator_prog = $(OUT_DIR)/generator

all: $(main_prog) $(students_prog) $(generator_prog)

$(main_prog) : $(main_objects)
	@$(CC) $(CFLAGS) $(main_objects) -o $@

$(students_prog) : $(students_objects)
	@$(CC) $(CFLAGS) $(students_objects) -o $@

$(generator_prog) : $(generator_objects)
	@$(CC) $(CFLAGS) $(generator_objects) -o $@
	
$(OUT_DIR)/%.o : %.c
	@$(CC) -c $(CFLAGS) $^ -o $@

.PHONY: clean
clean:
	@rm -rf $(DEBUG)/* $(RELEASE)/* test