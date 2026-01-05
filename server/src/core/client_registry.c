#include "server.h"

extern client_entry_t client_registry[MAX_CLIENTS];
extern pthread_mutex_t registry_mutex;

void init_client_registry() {
    pthread_mutex_lock(&registry_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_registry[i].socket_fd = -1;
        client_registry[i].user_id = 0;
        client_registry[i].is_active = 0;
    }
    pthread_mutex_unlock(&registry_mutex);
    printf("Client registry initialized.\n");
}

void register_client(int socket_fd, int user_id) {
    pthread_mutex_lock(&registry_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!client_registry[i].is_active) {
            client_registry[i].socket_fd = socket_fd;
            client_registry[i].user_id = user_id;
            client_registry[i].is_active = 1;
            printf("Registered client: socket=%d, user_id=%d\n", socket_fd, user_id);
            break;
        }
    }
    pthread_mutex_unlock(&registry_mutex);
}

void unregister_client(int socket_fd) {
    pthread_mutex_lock(&registry_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_registry[i].socket_fd == socket_fd && client_registry[i].is_active) {
            printf("Unregistered client: socket=%d, user_id=%d\n", 
                   socket_fd, client_registry[i].user_id);
            client_registry[i].socket_fd = -1;
            client_registry[i].user_id = 0;
            client_registry[i].is_active = 0;
            break;
        }
    }
    pthread_mutex_unlock(&registry_mutex);
}

int find_client_socket(int user_id) {
    int socket_fd = -1;
    pthread_mutex_lock(&registry_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_registry[i].user_id == user_id && client_registry[i].is_active) {
            socket_fd = client_registry[i].socket_fd;
            break;
        }
    }
    pthread_mutex_unlock(&registry_mutex);
    return socket_fd;
}
