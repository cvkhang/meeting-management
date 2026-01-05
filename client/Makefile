CC = gcc
CFLAGS = -Wall -Iinclude
LDFLAGS = -lpthread

SRC = src/main.c src/network.c src/ui.c
OBJ = $(SRC:.c=.o)
TARGET = build/client

all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p build
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o build/client
