CC = gcc
CFLAGS = -Wall -Wextra -Werror
BIN_DIR = bin
SRC_DIR = src

all: $(BIN_DIR)/tracer $(BIN_DIR)/monitor

$(BIN_DIR)/tracer: $(SRC_DIR)/tracer.c
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR)/monitor: $(SRC_DIR)/monitor.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(BIN_DIR)/*
