#include "server.h"
#include <time.h>

extern sqlite3 *db;

// Helper to log and send response
void send_response(int socket_fd, const char *response) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
    
    // Remove \r\n for cleaner log
    char log_msg[4096];
    strncpy(log_msg, response, sizeof(log_msg) - 1);
    log_msg[strcspn(log_msg, "\r\n")] = '\0';
    
    printf("[%s] [SEND] %s\n", timestamp, log_msg);
    send(socket_fd, response, strlen(response), 0);
}

// Helper to get value by key from payload string "key=value;key2=value2"
// Returns a newly allocated string that must be freed, or NULL if not found.
char *get_value(char *payload, const char *key) {
    if (!payload) return NULL;
    
    char *payload_copy = strdup(payload);
    // Remove trailing \r\n
    payload_copy[strcspn(payload_copy, "\r\n")] = 0;
    
    char *token = strtok(payload_copy, ";");
    char *result = NULL;

    while (token != NULL) {
        char *eq = strchr(token, '=');
        if (eq) {
            *eq = '\0';
            char *k = token;
            char *v = eq + 1;
            if (strcmp(k, key) == 0) {
                result = strdup(v);
                break;
            }
        }
        token = strtok(NULL, ";");
    }
    free(payload_copy);
    return result;
}

// Save notification to database for offline user
void save_pending_notification(int user_id, const char *type, const char *payload) {
    const char *sql = "INSERT INTO pending_notifications (user_id, type, payload) VALUES (?, ?, ?)";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        sqlite3_bind_text(stmt, 2, type, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, payload ? payload : "", -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        printf("[NOTI] Saved pending notification for user %d: %s\n", user_id, type);
    }
}

// Send pending notifications when user comes online
void send_pending_notifications(int socket_fd, int user_id) {
    const char *sql = "SELECT id, type, payload FROM pending_notifications WHERE user_id = ?";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return;
    
    sqlite3_bind_int(stmt, 1, user_id);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char *type = (const char *)sqlite3_column_text(stmt, 1);
        const char *payload = (const char *)sqlite3_column_text(stmt, 2);
        
        char notification[BUFFER_SIZE];
        snprintf(notification, sizeof(notification), "NTF|type=%s;%s\r\n", type, payload ? payload : "");
        send_response(socket_fd, notification);
        
        // Delete after sending
        char delete_sql[128];
        snprintf(delete_sql, sizeof(delete_sql), "DELETE FROM pending_notifications WHERE id = %d", id);
        sqlite3_exec(db, delete_sql, NULL, NULL, NULL);
    }
    sqlite3_finalize(stmt);
}

// Send notification to user (or save if offline)
void send_notification(int user_id, const char *type, const char *payload) {
    int socket_fd = find_client_socket(user_id);
    
    if (socket_fd > 0) {
        // User is online, send immediately
        char notification[BUFFER_SIZE];
        snprintf(notification, sizeof(notification), "NTF|type=%s;%s\r\n", type, payload ? payload : "");
        send_response(socket_fd, notification);
        printf("[NOTI] Sent notification to user %d: %s\n", user_id, type);
    } else {
        // User is offline, save to database
        save_pending_notification(user_id, type, payload);
    }
}
