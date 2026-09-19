CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
LDFLAGS = -lreadline

SRC = main.c src/py_venv.c src/git.c
OBJ = $(SRC:.c=.o)
BIN = dsh
PREFIX = /usr/local

.PHONY: all install uninstall clean

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(BIN) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(BIN)
	install -Dm755 $(BIN) $(PREFIX)/bin/$(BIN)

uninstall:
	rm -f $(PREFIX)/bin/$(BIN)

clean:
	rm -f $(OBJ) $(BIN)
