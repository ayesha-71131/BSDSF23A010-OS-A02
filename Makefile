# Simple Makefile for ls-v1.1.0
CC = gcc
CFLAGS = -Wall
SRC = src/ls-v1.1.0.c
BIN = bin/ls

all: $(BIN)

$(BIN): $(SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

clean:
	rm -f $(BIN)

