CC = gcc
CFLAGS = -Wall -Iinclude
LDFLAGS = -lsqlite3 -lpthread

SRC = src/main.c src/router.c \
      src/core/client_registry.c \
      src/core/database.c \
      src/utils/utils.c \
      src/handlers/auth_handler.c \
      src/handlers/slot_handler.c \
      src/handlers/meeting_handler.c \
      src/handlers/group_handler.c \
      src/handlers/minutes_handler.c
OBJ = $(SRC:.c=.o)
TARGET = build/server

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o src/core/*.o src/utils/*.o src/handlers/*.o build/server
