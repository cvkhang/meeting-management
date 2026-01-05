#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sqlite3.h>

#define PORT 1234
#define BUFFER_SIZE 4096
#define DB_FILE "meeting.db"
#define MAX_CLIENTS 100

typedef struct {
    int socket_fd;
    struct sockaddr_in address;
} client_t;

// Client registry for tracking online users
typedef struct {
    int socket_fd;
    int user_id;
    int is_active;
} client_entry_t;

extern client_entry_t client_registry[MAX_CLIENTS];
extern pthread_mutex_t registry_mutex;

// Notification functions
void send_notification(int user_id, const char *type, const char *payload);
void send_pending_notifications(int socket_fd, int user_id);
void save_pending_notification(int user_id, const char *type, const char *payload);

void *handle_client(void *arg);
void process_command(int socket_fd, char *buffer);

// Include core modules
#include "core/client_registry.h"
#include "core/database.h"

// Include modular headers
#include "handlers/auth_handler.h"
#include "handlers/slot_handler.h"
#include "handlers/meeting_handler.h"
#include "handlers/group_handler.h"
#include "handlers/minutes_handler.h"
#include "utils/utils.h"

#endif // SERVER_H
