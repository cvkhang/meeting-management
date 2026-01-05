#include "server.h"
#include "utils/utils.h"

extern sqlite3 *db;

void handle_register(int socket_fd, char *payload) {
    char *role = get_value(payload, "role");
    char *username = get_value(payload, "username");
    char *password = get_value(payload, "password");
    char *full_name = get_value(payload, "full_name");

    if (!role || !username || !password || !full_name) {
        char *response = "400|msg=Missing_fields\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    const char *sql = "INSERT INTO users (username, password, full_name, role) VALUES (?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        char *response = "500|msg=Internal_Server_Error\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, full_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, role, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        // Likely username exists
        char response[256];
        snprintf(response, sizeof(response), "409|msg=Username_exists\r\n");
        send_response(socket_fd, response);
    } else {
        int user_id = (int)sqlite3_last_insert_rowid(db);
        char response[256];
        snprintf(response, sizeof(response), "201|user_id=%d;msg=Registered\r\n", user_id);
        send_response(socket_fd, response);
    }
    
    sqlite3_finalize(stmt);

cleanup:
    if (role) free(role);
    if (username) free(username);
    if (password) free(password);
    if (full_name) free(full_name);
}

void handle_login(int socket_fd, char *payload) {
    char *username = get_value(payload, "username");
    char *password = get_value(payload, "password");
    
    if (!username || !password) {
        char *response = "400|msg=Missing_fields\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    // Auto-detect role from database
    const char *sql = "SELECT user_id, role FROM users WHERE username = ? AND password = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        char *response = "500|msg=Internal_Server_Error\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int user_id = sqlite3_column_int(stmt, 0);
        const char *role = (const char *)sqlite3_column_text(stmt, 1);
        
        // Generate a simple token (in real app, use UUID or JWT)
        char token[64];
        snprintf(token, sizeof(token), "token_%d_%s", user_id, role);
        
        // Register client in the registry
        register_client(socket_fd, user_id);
        
        char response[512];
        snprintf(response, sizeof(response), "200|user_id=%d;role=%s;token=%s;msg=Login_OK\r\n", user_id, role, token);
        send_response(socket_fd, response);
        
        // Send any pending notifications
        send_pending_notifications(socket_fd, user_id);
    } else {
        char *response = "404|msg=User_not_found_or_Wrong_password\r\n";
        send_response(socket_fd, response);
    }

    sqlite3_finalize(stmt);

cleanup:
    if (username) free(username);
    if (password) free(password);
}
