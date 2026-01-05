#ifndef UTILS_H
#define UTILS_H

#include <sqlite3.h>

// Helper functions
void send_response(int socket_fd, const char *response);
char *get_value(char *payload, const char *key);

// Notification functions
void save_pending_notification(int user_id, const char *type, const char *payload);
void send_pending_notifications(int socket_fd, int user_id);
void send_notification(int user_id, const char *type, const char *payload);

#endif // UTILS_H
