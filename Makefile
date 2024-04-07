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

run: all
	@echo $(MSG_INFO)
	@./$(main_prog) $(main_args)

all: $(main_prog) $(producer_prog) $(consumer_prog)

$(main_prog) : $(main_objects)
	@$(CC) $(CFLAGS) $(main_objects) -o $@

$(producer_prog) : $(producer_objects)
	@$(CC) $(CFLAGS) $(producer_objects) -o $@

$(consumer_prog) : $(consumer_objects)
	@$(CC) $(CFLAGS) $(consumer_objects) -o $@
	
$(OUT_DIR)/%.o : %.c
	@$(CC) -c $(CFLAGS) $^ -o $@

.PHONY: clean
clean:
	@rm -rf $(DEBUG)/* $(RELEASE)/* test