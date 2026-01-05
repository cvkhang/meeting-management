#ifndef MINUTES_HANDLER_H
#define MINUTES_HANDLER_H

// Minutes management handlers
void handle_save_minutes(int socket_fd, char *payload);
void handle_update_minutes(int socket_fd, char *payload);
void handle_view_minutes(int socket_fd, char *payload);

#endif // MINUTES_HANDLER_H
