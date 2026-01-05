#include "server.h"

sqlite3 *db;
client_entry_t client_registry[MAX_CLIENTS];
pthread_mutex_t registry_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg) {
    client_t *client = (client_t *)arg;
    char buffer[BUFFER_SIZE];
    int n;

    printf("Client connected (socket=%d).\n", client->socket_fd);

    while ((n = recv(client->socket_fd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[n] = '\0';
        process_command(client->socket_fd, buffer);
    }

    if (n == 0) {
        printf("Client disconnected (socket=%d).\n", client->socket_fd);
    } else {
        perror("recv failed");
    }

    // Unregister client when disconnected
    unregister_client(client->socket_fd);
    close(client->socket_fd);
    free(client);
    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr;

    init_client_registry();
    init_db();

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Allow port reuse
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("====================================\n");
    printf("Server is running!\n");
    printf("====================================\n");
    printf("Port: %d\n", PORT);
    printf("\nAvailable network interfaces:\n");
    
    // Get all network interfaces
    system("hostname -I 2>/dev/null || ifconfig | grep 'inet ' | awk '{print $2}' | grep -v '127.0.0.1'");
    
    printf("\nClients can connect using:\n");
    printf("  - Localhost: 127.0.0.1:%d\n", PORT);
    printf("  - LAN IP: <IP from above>:%d\n", PORT);
    printf("====================================\n\n");
    printf("Waiting for connections...\n\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int *client_fd = malloc(sizeof(int));
        
        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (*client_fd < 0) {
            perror("accept failed");
            free(client_fd);
            continue;
        }

        client_t *client = malloc(sizeof(client_t));
        client->socket_fd = *client_fd;
        client->address = client_addr;
        free(client_fd);

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *)client) < 0) {
            perror("could not create thread");
            free(client);
            continue;
        }
        pthread_detach(thread_id);
    }

    return 0;
}
