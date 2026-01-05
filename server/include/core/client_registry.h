#ifndef CLIENT_REGISTRY_H
#define CLIENT_REGISTRY_H

// Client registry functions
void init_client_registry();
void register_client(int socket_fd, int user_id);
void unregister_client(int socket_fd);
int find_client_socket(int user_id);

#endif // CLIENT_REGISTRY_H
