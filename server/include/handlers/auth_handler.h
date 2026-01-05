#ifndef AUTH_HANDLER_H
#define AUTH_HANDLER_H

// Authentication handlers
void handle_register(int socket_fd, char *payload);
void handle_login(int socket_fd, char *payload);

#endif // AUTH_HANDLER_H
