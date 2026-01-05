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

void handle_create_group(int socket_fd, char *payload) {
    char *token = get_value(payload, "token");
    char *group_name = get_value(payload, "group_name");

    // TODO: Validate token (extract user_id from token for simplicity)
    // token format: token_<user_id>_<role>
    int user_id = 0;
    if (token) sscanf(token, "token_%d", &user_id);

    if (!token || !group_name || user_id == 0) {
        char *response = "401|msg=Invalid_token_or_Missing_fields\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    const char *sql = "INSERT INTO groups (group_name, created_by) VALUES (?, ?)";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, group_name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, user_id);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        int group_id = (int)sqlite3_last_insert_rowid(db);
        
        // Add creator as admin
        const char *sql2 = "INSERT INTO group_members (group_id, user_id, role) VALUES (?, ?, 1)";
        sqlite3_stmt *stmt2;
        sqlite3_prepare_v2(db, sql2, -1, &stmt2, NULL);
        sqlite3_bind_int(stmt2, 1, group_id);
        sqlite3_bind_int(stmt2, 2, user_id);
        sqlite3_step(stmt2);
        sqlite3_finalize(stmt2);

        char response[256];
        snprintf(response, sizeof(response), "201|group_id=%d\r\n", group_id);
        send_response(socket_fd, response);
    } else {
        char *response = "500|msg=Create_Group_Failed\r\n";
        send_response(socket_fd, response);
    }
    sqlite3_finalize(stmt);

cleanup:
    if (token) free(token);
    if (group_name) free(group_name);
}

void handle_view_groups(int socket_fd, char *payload) {
    char *token = get_value(payload, "token");
    
    int user_id = 0;
    if (token) sscanf(token, "token_%d", &user_id);
    
    // Get all groups with member count, is_member flag, and role
    // is_member: 1 if user is in group, 0 if not
    // role: 1 = admin, 0 = member (only meaningful if is_member = 1)
    const char *sql = "SELECT g.group_id, g.group_name, "
                      "(SELECT COUNT(*) FROM group_members WHERE group_id = g.group_id) as member_count, "
                      "CASE WHEN EXISTS(SELECT 1 FROM group_members WHERE group_id = g.group_id AND user_id = ?) THEN 1 ELSE 0 END as is_member, "
                      "COALESCE((SELECT role FROM group_members WHERE group_id = g.group_id AND user_id = ?), 0) as role "
                      "FROM groups g";
    
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, user_id);

    char buffer[4096] = "200|groups=";
    int first = 1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) strcat(buffer, "#");
        int gid = sqlite3_column_int(stmt, 0);
        const unsigned char *name = sqlite3_column_text(stmt, 1);
        int count = sqlite3_column_int(stmt, 2);
        int is_member = sqlite3_column_int(stmt, 3);
        int role = sqlite3_column_int(stmt, 4);
        
        char item[256];
        // Format: group_id,group_name,member_count,is_member,role
        snprintf(item, sizeof(item), "%d,%s,%d,%d,%d", gid, name, count, is_member, role);
        strcat(buffer, item);
        first = 0;
    }
    strcat(buffer, "\r\n");
    send_response(socket_fd, buffer);
    sqlite3_finalize(stmt);
    
    if (token) free(token);
}

void process_command(int socket_fd, char *buffer) {
    // buffer format: COMMAND|payload\r\n
    buffer[strcspn(buffer, "\r\n")] = 0;

    // Log received message with timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
    printf("[%s] [RECV] %s\n", timestamp, buffer);

    char *command = strtok(buffer, "|");
    char *payload = strtok(NULL, ""); 

    if (!command) return;

    // Authentication
    if (strcmp(command, "REGISTER") == 0) {
        handle_register(socket_fd, payload);
    } else if (strcmp(command, "LOGIN") == 0) {
        handle_login(socket_fd, payload);
    }
    // Slot management
    else if (strcmp(command, "DECLARE_SLOT") == 0) {
        handle_declare_slot(socket_fd, payload);
    } else if (strcmp(command, "EDIT_SLOT") == 0) {
        handle_edit_slot(socket_fd, payload);
    } else if (strcmp(command, "VIEW_SLOTS") == 0) {
        handle_view_slots(socket_fd, payload);
    } else if (strcmp(command, "VIEW_TEACHERS") == 0) {
        handle_view_teachers(socket_fd, payload);
    }
    // Meeting management
    else if (strcmp(command, "VIEW_MEETINGS") == 0) {
        handle_view_meetings(socket_fd, payload);
    } else if (strcmp(command, "BOOK_MEETING_INDIV") == 0) {
        handle_book_meeting_indiv(socket_fd, payload);
    } else if (strcmp(command, "BOOK_MEETING_GROUP") == 0) {
        handle_book_meeting_group(socket_fd, payload);
    } else if (strcmp(command, "CANCEL_MEETING") == 0) {
        handle_cancel_meeting(socket_fd, payload);
    } else if (strcmp(command, "VIEW_MEETING_DETAIL") == 0) {
        handle_view_meeting_detail(socket_fd, payload);
    } else if (strcmp(command, "VIEW_MEETING_HISTORY") == 0) {
        handle_view_meeting_history(socket_fd, payload);
    }
    // Minutes management
    else if (strcmp(command, "SAVE_MINUTES") == 0) {
        handle_save_minutes(socket_fd, payload);
    } else if (strcmp(command, "UPDATE_MINUTES") == 0) {
        handle_update_minutes(socket_fd, payload);
    } else if (strcmp(command, "VIEW_MINUTES") == 0) {
        handle_view_minutes(socket_fd, payload);
    }
    // Group management
    else if (strcmp(command, "CREATE_GROUP") == 0) {
        handle_create_group(socket_fd, payload);
    } else if (strcmp(command, "VIEW_GROUPS") == 0) {
        handle_view_groups(socket_fd, payload);
    } else if (strcmp(command, "VIEW_GROUP_DETAIL") == 0) {
        handle_view_group_detail(socket_fd, payload);
    } else if (strcmp(command, "REQUEST_JOIN_GROUP") == 0) {
        handle_request_join_group(socket_fd, payload);
    } else if (strcmp(command, "VIEW_JOIN_REQUESTS") == 0) {
        handle_view_join_requests(socket_fd, payload);
    } else if (strcmp(command, "APPROVE_JOIN_REQUEST") == 0) {
        handle_approve_join_request(socket_fd, payload);
    } else if (strcmp(command, "REJECT_JOIN_REQUEST") == 0) {
        handle_reject_join_request(socket_fd, payload);
    }
    else {
        char *response = "400|msg=Unknown_command\r\n";
        send_response(socket_fd, response);
    }
}

void handle_declare_slot(int socket_fd, char *payload) {
    char *token = get_value(payload, "token");
    char *date = get_value(payload, "date");
    char *start_time = get_value(payload, "start_time");
    char *end_time = get_value(payload, "end_time");
    char *slot_type = get_value(payload, "slot_type");
    char *max_group_size_str = get_value(payload, "max_group_size");

    int teacher_id = 0;
    if (token) sscanf(token, "token_%d", &teacher_id);

    if (!token || !date || !start_time || !end_time || !slot_type || teacher_id == 0) {
        char *response = "400|msg=Missing_fields\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }
    
    int max_group_size = max_group_size_str ? atoi(max_group_size_str) : 1;

    const char *sql = "INSERT INTO slots (teacher_id, date, start_time, end_time, slot_type, max_group_size) VALUES (?, ?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, teacher_id);
    sqlite3_bind_text(stmt, 2, date, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, start_time, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, end_time, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, slot_type, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, max_group_size);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        int slot_id = (int)sqlite3_last_insert_rowid(db);
        char response[256];
        snprintf(response, sizeof(response), "201|slot_id=%d;msg=Slot_created\r\n", slot_id);
        send_response(socket_fd, response);
    } else {
        char *response = "500|msg=Create_Slot_Failed\r\n";
        send_response(socket_fd, response);
    }
    sqlite3_finalize(stmt);

cleanup:
    if (token) free(token);
    if (date) free(date);
    if (start_time) free(start_time);
    if (end_time) free(end_time);
    if (slot_type) free(slot_type);
    if (max_group_size_str) free(max_group_size_str);
}

void handle_view_slots(int socket_fd, char *payload) {
    char *teacher_id_str = get_value(payload, "teacher_id");
    // Assuming date range is optional or handled simply for now
    
    if (!teacher_id_str) {
        char *response = "400|msg=Missing_teacher_id\r\n";
        send_response(socket_fd, response);
        return;
    }

    const char *sql = "SELECT slot_id, date, start_time, end_time, slot_type, max_group_size FROM slots WHERE teacher_id = ?";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, atoi(teacher_id_str));

    char buffer[4096] = "200|slots=";
    int first = 1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) strcat(buffer, "#");
        int sid = sqlite3_column_int(stmt, 0);
        const unsigned char *date = sqlite3_column_text(stmt, 1);
        const unsigned char *st = sqlite3_column_text(stmt, 2);
        const unsigned char *et = sqlite3_column_text(stmt, 3);
        const unsigned char *type = sqlite3_column_text(stmt, 4);
        int max = sqlite3_column_int(stmt, 5);
        
        char item[256];
        snprintf(item, sizeof(item), "%d,%s,%s,%s,%s,%d", sid, date, st, et, type, max);
        strcat(buffer, item);
        first = 0;
    }
    strcat(buffer, "\r\n");
    send_response(socket_fd, buffer);
    sqlite3_finalize(stmt);
    free(teacher_id_str);
}

void handle_book_meeting_indiv(int socket_fd, char *payload) {
    char *token = get_value(payload, "token");
    char *teacher_id_str = get_value(payload, "teacher_id");
    char *slot_id_str = get_value(payload, "slot_id");

    int student_id = 0;
    if (token) sscanf(token, "token_%d", &student_id);

    if (!token || !teacher_id_str || !slot_id_str || student_id == 0) {
        char *response = "400|msg=Missing_fields\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    const char *sql = "INSERT INTO meetings (slot_id, teacher_id, student_id, meeting_type, status) VALUES (?, ?, ?, 'INDIVIDUAL', 'BOOKED')";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, atoi(slot_id_str));
    sqlite3_bind_int(stmt, 2, atoi(teacher_id_str));
    sqlite3_bind_int(stmt, 3, student_id);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        int meeting_id = (int)sqlite3_last_insert_rowid(db);
        char response[256];
        snprintf(response, sizeof(response), "201|slot_id=%s;meeting_id=%d;msg=Booked\r\n", slot_id_str, meeting_id);
        send_response(socket_fd, response);
        
        // Notify teacher about new booking
        char noti_payload[256];
        snprintf(noti_payload, sizeof(noti_payload), "meeting_id=%d;slot_id=%s;student_id=%d", 
                 meeting_id, slot_id_str, student_id);
        send_notification(atoi(teacher_id_str), "MEETING_BOOKED", noti_payload);
    } else {
        char *response = "500|msg=Booking_Failed\r\n";
        send_response(socket_fd, response);
    }
    sqlite3_finalize(stmt);

cleanup:
    if (token) free(token);
    if (teacher_id_str) free(teacher_id_str);
    if (slot_id_str) free(slot_id_str);
}

// EDIT_SLOT handler - UPDATE or DELETE a slot
void handle_edit_slot(int socket_fd, char *payload) {
    char *token = get_value(payload, "token");
    char *slot_id_str = get_value(payload, "slot_id");
    char *action = get_value(payload, "action");

    int teacher_id = 0;
    if (token) sscanf(token, "token_%d", &teacher_id);

    if (!token || !slot_id_str || !action || teacher_id == 0) {
        char *response = "400|msg=Missing_fields\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    int slot_id = atoi(slot_id_str);

    // Verify ownership
    const char *verify_sql = "SELECT slot_id FROM slots WHERE slot_id = ? AND teacher_id = ?";
    sqlite3_stmt *verify_stmt;
    sqlite3_prepare_v2(db, verify_sql, -1, &verify_stmt, NULL);
    sqlite3_bind_int(verify_stmt, 1, slot_id);
    sqlite3_bind_int(verify_stmt, 2, teacher_id);

    if (sqlite3_step(verify_stmt) != SQLITE_ROW) {
        char *response = "404|msg=Slot_not_found_or_Not_owner\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(verify_stmt);
        goto cleanup;
    }
    sqlite3_finalize(verify_stmt);

    if (strcmp(action, "DELETE") == 0) {
        const char *sql = "DELETE FROM slots WHERE slot_id = ?";
        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, slot_id);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            char response[256];
            snprintf(response, sizeof(response), "200|slot_id=%d;msg=Slot_deleted\r\n", slot_id);
            send_response(socket_fd, response);
        } else {
            char *response = "500|msg=Delete_Failed\r\n";
            send_response(socket_fd, response);
        }
        sqlite3_finalize(stmt);
    } else if (strcmp(action, "UPDATE") == 0) {
        char *date = get_value(payload, "date");
        char *start_time = get_value(payload, "start_time");
        char *end_time = get_value(payload, "end_time");
        char *slot_type = get_value(payload, "slot_type");
        char *max_group_size_str = get_value(payload, "max_group_size");

        if (!date || !start_time || !end_time || !slot_type) {
            char *response = "400|msg=Missing_fields\r\n";
            send_response(socket_fd, response);
        } else {
            int max_group_size = max_group_size_str ? atoi(max_group_size_str) : 1;
            const char *sql = "UPDATE slots SET date=?, start_time=?, end_time=?, slot_type=?, max_group_size=? WHERE slot_id=?";
            sqlite3_stmt *stmt;
            sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
            sqlite3_bind_text(stmt, 1, date, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, start_time, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, end_time, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 4, slot_type, -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 5, max_group_size);
            sqlite3_bind_int(stmt, 6, slot_id);

            if (sqlite3_step(stmt) == SQLITE_DONE) {
                char response[256];
                snprintf(response, sizeof(response), "200|slot_id=%d;msg=Slot_updated\r\n", slot_id);
                send_response(socket_fd, response);
            } else {
                char *response = "500|msg=Update_Failed\r\n";
                send_response(socket_fd, response);
            }
            sqlite3_finalize(stmt);
        }

        if (date) free(date);
        if (start_time) free(start_time);
        if (end_time) free(end_time);
        if (slot_type) free(slot_type);
        if (max_group_size_str) free(max_group_size_str);
    }

cleanup:
    if (token) free(token);
    if (slot_id_str) free(slot_id_str);
    if (action) free(action);
}

// VIEW_TEACHERS handler
void handle_view_teachers(int socket_fd, char *payload) {
    const char *sql = 
        "SELECT u.user_id, u.full_name, "
        "(SELECT COUNT(*) FROM slots s WHERE s.teacher_id = u.user_id "
        "AND s.slot_id NOT IN (SELECT slot_id FROM meetings WHERE status = 'BOOKED')) as slot_count "
        "FROM users u WHERE u.role = 'TEACHER'";
    
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    char buffer[4096] = "200|teachers=";
    int first = 1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) strcat(buffer, "#");
        int uid = sqlite3_column_int(stmt, 0);
        const unsigned char *name = sqlite3_column_text(stmt, 1);
        int count = sqlite3_column_int(stmt, 2);
        
        char item[256];
        snprintf(item, sizeof(item), "%d,%s,%d", uid, name, count);
        strcat(buffer, item);
        first = 0;
    }
    strcat(buffer, "\r\n");
    send_response(socket_fd, buffer);
    sqlite3_finalize(stmt);
}

// VIEW_MEETINGS handler
void handle_view_meetings(int socket_fd, char *payload) {
    char *token = get_value(payload, "token");
    char *from_date = get_value(payload, "from_date");
    char *to_date = get_value(payload, "to_date");
    char *status = get_value(payload, "status");

    int user_id = 0;
    char role[16] = "";
    if (token) sscanf(token, "token_%d_%15s", &user_id, role);

    if (!token || user_id == 0) {
        char *response = "401|msg=Invalid_token\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    char sql[512];
    if (strcmp(role, "TEACHER") == 0) {
        snprintf(sql, sizeof(sql),
            "SELECT m.meeting_id, s.date, s.start_time, s.end_time, m.slot_id, "
            "COALESCE(m.student_id, m.group_id), m.meeting_type, m.status "
            "FROM meetings m JOIN slots s ON m.slot_id = s.slot_id "
            "WHERE m.teacher_id = ? AND (%s IS NULL OR s.date >= ?) AND (%s IS NULL OR s.date <= ?) "
            "AND (%s IS NULL OR m.status = ?)",
            from_date ? "1" : "NULL", to_date ? "1" : "NULL", status ? "1" : "NULL");
    } else {
        snprintf(sql, sizeof(sql),
            "SELECT m.meeting_id, s.date, s.start_time, s.end_time, m.slot_id, "
            "m.teacher_id, m.meeting_type, m.status "
            "FROM meetings m JOIN slots s ON m.slot_id = s.slot_id "
            "WHERE (m.student_id = ? OR m.group_id IN (SELECT group_id FROM group_members WHERE user_id = ?)) "
            "AND (%s IS NULL OR s.date >= ?) AND (%s IS NULL OR s.date <= ?) "
            "AND (%s IS NULL OR m.status = ?)",
            from_date ? "1" : "NULL", to_date ? "1" : "NULL", status ? "1" : "NULL");
    }

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    int param = 1;
    sqlite3_bind_int(stmt, param++, user_id);
    if (strcmp(role, "STUDENT") == 0) sqlite3_bind_int(stmt, param++, user_id);
    if (from_date) sqlite3_bind_text(stmt, param++, from_date, -1, SQLITE_STATIC);
    if (to_date) sqlite3_bind_text(stmt, param++, to_date, -1, SQLITE_STATIC);
    if (status) sqlite3_bind_text(stmt, param++, status, -1, SQLITE_STATIC);

    char buffer[4096] = "200|meetings=";
    int first = 1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) strcat(buffer, "#");
        char item[256];
        snprintf(item, sizeof(item), "%d,%s,%s,%s,%d,%d,%s,%s",
            sqlite3_column_int(stmt, 0),
            sqlite3_column_text(stmt, 1),
            sqlite3_column_text(stmt, 2),
            sqlite3_column_text(stmt, 3),
            sqlite3_column_int(stmt, 4),
            sqlite3_column_int(stmt, 5),
            sqlite3_column_text(stmt, 6),
            sqlite3_column_text(stmt, 7));
        strcat(buffer, item);
        first = 0;
    }
    strcat(buffer, "\r\n");
    send_response(socket_fd, buffer);
    sqlite3_finalize(stmt);

cleanup:
    if (token) free(token);
    if (from_date) free(from_date);
    if (to_date) free(to_date);
    if (status) free(status);
}

// BOOK_MEETING_GROUP handler
void handle_book_meeting_group(int socket_fd, char *payload) {
    char *token = get_value(payload, "token");
    char *teacher_id_str = get_value(payload, "teacher_id");
    char *slot_id_str = get_value(payload, "slot_id");
    char *group_id_str = get_value(payload, "group_id");

    int user_id = 0;
    if (token) sscanf(token, "token_%d", &user_id);

    if (!token || !teacher_id_str || !slot_id_str || !group_id_str || user_id == 0) {
        char *response = "400|msg=Missing_fields\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    int group_id = atoi(group_id_str);

    // Check if user is admin of the group
    const char *check_sql = "SELECT role FROM group_members WHERE group_id = ? AND user_id = ?";
    sqlite3_stmt *check_stmt;
    sqlite3_prepare_v2(db, check_sql, -1, &check_stmt, NULL);
    sqlite3_bind_int(check_stmt, 1, group_id);
    sqlite3_bind_int(check_stmt, 2, user_id);

    if (sqlite3_step(check_stmt) != SQLITE_ROW || sqlite3_column_int(check_stmt, 0) != 1) {
        char *response = "403|msg=Not_group_admin\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(check_stmt);
        goto cleanup;
    }
    sqlite3_finalize(check_stmt);

    // Check if slot is already booked
    int slot_id = atoi(slot_id_str);
    const char *slot_check = "SELECT slot_id FROM meetings WHERE slot_id = ? AND status = 'BOOKED'";
    sqlite3_stmt *slot_stmt;
    sqlite3_prepare_v2(db, slot_check, -1, &slot_stmt, NULL);
    sqlite3_bind_int(slot_stmt, 1, slot_id);

    if (sqlite3_step(slot_stmt) == SQLITE_ROW) {
        char *response = "409|msg=Slot_already_booked\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(slot_stmt);
        goto cleanup;
    }
    sqlite3_finalize(slot_stmt);

    const char *sql = "INSERT INTO meetings (slot_id, teacher_id, group_id, meeting_type, status) VALUES (?, ?, ?, 'GROUP', 'BOOKED')";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, slot_id);
    sqlite3_bind_int(stmt, 2, atoi(teacher_id_str));
    sqlite3_bind_int(stmt, 3, group_id);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        int meeting_id = (int)sqlite3_last_insert_rowid(db);
        char response[256];
        snprintf(response, sizeof(response), "201|slot_id=%s;meeting_id=%d;msg=Booked\r\n", slot_id_str, meeting_id);
        send_response(socket_fd, response);
    } else {
        char *response = "500|msg=Booking_Failed\r\n";
        send_response(socket_fd, response);
    }
    sqlite3_finalize(stmt);

cleanup:
    if (token) free(token);
    if (teacher_id_str) free(teacher_id_str);
    if (slot_id_str) free(slot_id_str);
    if (group_id_str) free(group_id_str);
}

// CANCEL_MEETING handler
void handle_cancel_meeting(int socket_fd, char *payload) {
    char *token = get_value(payload, "token");
    char *meeting_id_str = get_value(payload, "meeting_id");
    char *reason = get_value(payload, "reason");

    int user_id = 0;
    if (token) sscanf(token, "token_%d", &user_id);

    if (!token || !meeting_id_str || user_id == 0) {
        char *response = "400|msg=Missing_fields\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    int meeting_id = atoi(meeting_id_str);

    // Check if meeting exists and user is participant
    const char *check_sql = "SELECT m.teacher_id, m.student_id, m.group_id, s.date FROM meetings m "
                            "JOIN slots s ON m.slot_id = s.slot_id WHERE m.meeting_id = ?";
    sqlite3_stmt *check_stmt;
    sqlite3_prepare_v2(db, check_sql, -1, &check_stmt, NULL);
    sqlite3_bind_int(check_stmt, 1, meeting_id);

    if (sqlite3_step(check_stmt) != SQLITE_ROW) {
        char *response = "404|msg=Meeting_not_found\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(check_stmt);
        goto cleanup;
    }

    int teacher_id = sqlite3_column_int(check_stmt, 0);
    int student_id = sqlite3_column_int(check_stmt, 1);
    int group_id = sqlite3_column_int(check_stmt, 2);
    sqlite3_finalize(check_stmt);

    // Check if user is participant
    int is_participant = (user_id == teacher_id || user_id == student_id);
    if (!is_participant && group_id > 0) {
        const char *grp_sql = "SELECT 1 FROM group_members WHERE group_id = ? AND user_id = ?";
        sqlite3_stmt *grp_stmt;
        sqlite3_prepare_v2(db, grp_sql, -1, &grp_stmt, NULL);
        sqlite3_bind_int(grp_stmt, 1, group_id);
        sqlite3_bind_int(grp_stmt, 2, user_id);
        is_participant = (sqlite3_step(grp_stmt) == SQLITE_ROW);
        sqlite3_finalize(grp_stmt);
    }

    if (!is_participant) {
        char *response = "403|msg=Not_participant\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    // Cancel the meeting
    const char *sql = "UPDATE meetings SET status = 'CANCELLED' WHERE meeting_id = ?";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, meeting_id);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        char response[256];
        snprintf(response, sizeof(response), "200|meeting_id=%d;msg=Cancelled\r\n", meeting_id);
        send_response(socket_fd, response);
        
        // Notify the other party
        char noti_payload[256];
        snprintf(noti_payload, sizeof(noti_payload), "meeting_id=%d;cancelled_by=%d", meeting_id, user_id);
        if (user_id == teacher_id && student_id > 0) {
            send_notification(student_id, "MEETING_CANCELLED", noti_payload);
        } else if (user_id == student_id) {
            send_notification(teacher_id, "MEETING_CANCELLED", noti_payload);
        }
    } else {
        char *response = "500|msg=Cancel_Failed\r\n";
        send_response(socket_fd, response);
    }
    sqlite3_finalize(stmt);

cleanup:
    if (token) free(token);
    if (meeting_id_str) free(meeting_id_str);
    if (reason) free(reason);
}

// VIEW_MEETING_DETAIL handler
void handle_view_meeting_detail(int socket_fd, char *payload) {
    char *token = get_value(payload, "token");
    char *meeting_id_str = get_value(payload, "meeting_id");

    int user_id = 0;
    if (token) sscanf(token, "token_%d", &user_id);

    if (!token || !meeting_id_str || user_id == 0) {
        char *response = "400|msg=Missing_fields\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    int meeting_id = atoi(meeting_id_str);

    const char *sql = 
        "SELECT m.meeting_id, s.date, s.start_time, s.end_time, m.teacher_id, t.full_name, "
        "m.student_id, m.group_id, m.meeting_type, m.status, "
        "(SELECT COUNT(*) FROM meeting_minutes mm WHERE mm.meeting_id = m.meeting_id) as has_minutes "
        "FROM meetings m "
        "JOIN slots s ON m.slot_id = s.slot_id "
        "JOIN users t ON m.teacher_id = t.user_id "
        "WHERE m.meeting_id = ?";
    
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, meeting_id);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        char *response = "404|msg=Meeting_not_found\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(stmt);
        goto cleanup;
    }

    int teacher_id = sqlite3_column_int(stmt, 4);
    int student_id = sqlite3_column_int(stmt, 6);
    int group_id = sqlite3_column_int(stmt, 7);
    int has_minutes = sqlite3_column_int(stmt, 10);

    // Get partner name
    char partner_name[128] = "";
    int partner_id = 0;
    if (group_id > 0) {
        const char *grp_sql = "SELECT group_name FROM groups WHERE group_id = ?";
        sqlite3_stmt *grp_stmt;
        sqlite3_prepare_v2(db, grp_sql, -1, &grp_stmt, NULL);
        sqlite3_bind_int(grp_stmt, 1, group_id);
        if (sqlite3_step(grp_stmt) == SQLITE_ROW) {
            strncpy(partner_name, (const char *)sqlite3_column_text(grp_stmt, 0), 127);
        }
        sqlite3_finalize(grp_stmt);
        partner_id = group_id;
    } else if (student_id > 0) {
        const char *stu_sql = "SELECT full_name FROM users WHERE user_id = ?";
        sqlite3_stmt *stu_stmt;
        sqlite3_prepare_v2(db, stu_sql, -1, &stu_stmt, NULL);
        sqlite3_bind_int(stu_stmt, 1, student_id);
        if (sqlite3_step(stu_stmt) == SQLITE_ROW) {
            strncpy(partner_name, (const char *)sqlite3_column_text(stu_stmt, 0), 127);
        }
        sqlite3_finalize(stu_stmt);
        partner_id = student_id;
    }

    char response[1024];
    snprintf(response, sizeof(response),
        "200|meeting_id=%d;date=%s;start_time=%s;end_time=%s;teacher_id=%d;teacher_name=%s;"
        "%s=%d;%s=%s;meeting_type=%s;status=%s;has_minutes=%d\r\n",
        sqlite3_column_int(stmt, 0),
        sqlite3_column_text(stmt, 1),
        sqlite3_column_text(stmt, 2),
        sqlite3_column_text(stmt, 3),
        teacher_id,
        sqlite3_column_text(stmt, 5),
        group_id > 0 ? "group_id" : "student_id",
        partner_id,
        group_id > 0 ? "group_name" : "student_name",
        partner_name,
        sqlite3_column_text(stmt, 8),
        sqlite3_column_text(stmt, 9),
        has_minutes);
    send_response(socket_fd, response);
    sqlite3_finalize(stmt);

cleanup:
    if (token) free(token);
    if (meeting_id_str) free(meeting_id_str);
}

// VIEW_MEETING_HISTORY handler
void handle_view_meeting_history(int socket_fd, char *payload) {
    char *token = get_value(payload, "token");

    int user_id = 0;
    char role[16] = "";
    if (token) sscanf(token, "token_%d_%15s", &user_id, role);

    if (!token || user_id == 0) {
        char *response = "401|msg=Invalid_token\r\n";
        send_response(socket_fd, response);
        if (token) free(token);
        return;
    }

    const char *sql;
    if (strcmp(role, "TEACHER") == 0) {
        sql = "SELECT m.meeting_id, s.date, s.start_time, s.end_time, "
              "COALESCE(u.full_name, g.group_name) as partner_name, m.meeting_type, "
              "(SELECT COUNT(*) FROM meeting_minutes mm WHERE mm.meeting_id = m.meeting_id) as has_minutes "
              "FROM meetings m "
              "JOIN slots s ON m.slot_id = s.slot_id "
              "LEFT JOIN users u ON m.student_id = u.user_id "
              "LEFT JOIN groups g ON m.group_id = g.group_id "
              "WHERE m.teacher_id = ? AND m.status = 'DONE'";
    } else {
        sql = "SELECT m.meeting_id, s.date, s.start_time, s.end_time, "
              "t.full_name as partner_name, m.meeting_type, "
              "(SELECT COUNT(*) FROM meeting_minutes mm WHERE mm.meeting_id = m.meeting_id) as has_minutes "
              "FROM meetings m "
              "JOIN slots s ON m.slot_id = s.slot_id "
              "JOIN users t ON m.teacher_id = t.user_id "
              "WHERE (m.student_id = ? OR m.group_id IN (SELECT group_id FROM group_members WHERE user_id = ?)) "
              "AND m.status = 'DONE'";
    }

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, user_id);
    if (strcmp(role, "STUDENT") == 0) sqlite3_bind_int(stmt, 2, user_id);

    char buffer[4096] = "200|history=";
    int first = 1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) strcat(buffer, "#");
        char item[256];
        snprintf(item, sizeof(item), "%d,%s,%s,%s,%s,%s,%d",
            sqlite3_column_int(stmt, 0),
            sqlite3_column_text(stmt, 1),
            sqlite3_column_text(stmt, 2),
            sqlite3_column_text(stmt, 3),
            sqlite3_column_text(stmt, 4),
            sqlite3_column_text(stmt, 5),
            sqlite3_column_int(stmt, 6));
        strcat(buffer, item);
        first = 0;
    }
    strcat(buffer, "\r\n");
    send_response(socket_fd, buffer);
    sqlite3_finalize(stmt);
    free(token);
}

// SAVE_MINUTES handler
void handle_save_minutes(int socket_fd, char *payload) {
    char *token = get_value(payload, "token");
    char *meeting_id_str = get_value(payload, "meeting_id");
    char *content = get_value(payload, "content");

    int user_id = 0;
    if (token) sscanf(token, "token_%d", &user_id);

    if (!token || !meeting_id_str || !content || user_id == 0) {
        char *response = "400|msg=Missing_fields\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    int meeting_id = atoi(meeting_id_str);

    // Check if user is the teacher of this meeting
    const char *check_sql = "SELECT teacher_id FROM meetings WHERE meeting_id = ?";
    sqlite3_stmt *check_stmt;
    sqlite3_prepare_v2(db, check_sql, -1, &check_stmt, NULL);
    sqlite3_bind_int(check_stmt, 1, meeting_id);

    if (sqlite3_step(check_stmt) != SQLITE_ROW) {
        char *response = "404|msg=Meeting_not_found\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(check_stmt);
        goto cleanup;
    }

    if (sqlite3_column_int(check_stmt, 0) != user_id) {
        char *response = "403|msg=Not_meeting_teacher\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(check_stmt);
        goto cleanup;
    }
    sqlite3_finalize(check_stmt);

    // Check if minutes already exist
    const char *exist_sql = "SELECT minute_id FROM meeting_minutes WHERE meeting_id = ?";
    sqlite3_stmt *exist_stmt;
    sqlite3_prepare_v2(db, exist_sql, -1, &exist_stmt, NULL);
    sqlite3_bind_int(exist_stmt, 1, meeting_id);

    if (sqlite3_step(exist_stmt) == SQLITE_ROW) {
        char *response = "409|msg=Minutes_already_exists\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(exist_stmt);
        goto cleanup;
    }
    sqlite3_finalize(exist_stmt);

    const char *sql = "INSERT INTO meeting_minutes (meeting_id, content) VALUES (?, ?)";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, meeting_id);
    sqlite3_bind_text(stmt, 2, content, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        int minute_id = (int)sqlite3_last_insert_rowid(db);
        char response[256];
        snprintf(response, sizeof(response), "201|minute_id=%d;meeting_id=%d;msg=Minutes_saved\r\n", minute_id, meeting_id);
        send_response(socket_fd, response);
    } else {
        char *response = "500|msg=Save_Failed\r\n";
        send_response(socket_fd, response);
    }
    sqlite3_finalize(stmt);

cleanup:
    if (token) free(token);
    if (meeting_id_str) free(meeting_id_str);
    if (content) free(content);
}

// UPDATE_MINUTES handler
void handle_update_minutes(int socket_fd, char *payload) {
    char *token = get_value(payload, "token");
    char *minute_id_str = get_value(payload, "minute_id");
    char *content = get_value(payload, "content");

    int user_id = 0;
    if (token) sscanf(token, "token_%d", &user_id);

    if (!token || !minute_id_str || !content || user_id == 0) {
        char *response = "400|msg=Missing_fields\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    int minute_id = atoi(minute_id_str);

    // Check if minute exists and user is the teacher
    const char *check_sql = "SELECT m.teacher_id FROM meeting_minutes mm "
                            "JOIN meetings m ON mm.meeting_id = m.meeting_id "
                            "WHERE mm.minute_id = ?";
    sqlite3_stmt *check_stmt;
    sqlite3_prepare_v2(db, check_sql, -1, &check_stmt, NULL);
    sqlite3_bind_int(check_stmt, 1, minute_id);

    if (sqlite3_step(check_stmt) != SQLITE_ROW) {
        char *response = "404|msg=Minute_not_found\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(check_stmt);
        goto cleanup;
    }

    if (sqlite3_column_int(check_stmt, 0) != user_id) {
        char *response = "403|msg=Not_meeting_teacher\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(check_stmt);
        goto cleanup;
    }
    sqlite3_finalize(check_stmt);

    const char *sql = "UPDATE meeting_minutes SET content = ?, updated_at = CURRENT_TIMESTAMP WHERE minute_id = ?";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, content, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, minute_id);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        char response[256];
        snprintf(response, sizeof(response), "200|minute_id=%d;msg=Minutes_updated\r\n", minute_id);
        send_response(socket_fd, response);
    } else {
        char *response = "500|msg=Update_Failed\r\n";
        send_response(socket_fd, response);
    }
    sqlite3_finalize(stmt);

cleanup:
    if (token) free(token);
    if (minute_id_str) free(minute_id_str);
    if (content) free(content);
}

// VIEW_MINUTES handler
void handle_view_minutes(int socket_fd, char *payload) {
    char *token = get_value(payload, "token");
    char *meeting_id_str = get_value(payload, "meeting_id");

    int user_id = 0;
    if (token) sscanf(token, "token_%d", &user_id);

    if (!token || !meeting_id_str || user_id == 0) {
        char *response = "400|msg=Missing_fields\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    int meeting_id = atoi(meeting_id_str);

    // Check if meeting exists
    const char *check_sql = "SELECT teacher_id, student_id, group_id FROM meetings WHERE meeting_id = ?";
    sqlite3_stmt *check_stmt;
    sqlite3_prepare_v2(db, check_sql, -1, &check_stmt, NULL);
    sqlite3_bind_int(check_stmt, 1, meeting_id);

    if (sqlite3_step(check_stmt) != SQLITE_ROW) {
        char *response = "404|msg=Meeting_not_found\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(check_stmt);
        goto cleanup;
    }
    sqlite3_finalize(check_stmt);

    // Get minutes
    const char *sql = "SELECT minute_id, content, created_at, updated_at FROM meeting_minutes WHERE meeting_id = ?";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, meeting_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        char response[4096];
        snprintf(response, sizeof(response), "200|minute_id=%d;content=%s;created_at=%s;updated_at=%s\r\n",
            sqlite3_column_int(stmt, 0),
            sqlite3_column_text(stmt, 1),
            sqlite3_column_text(stmt, 2),
            sqlite3_column_text(stmt, 3));
        send_response(socket_fd, response);
    } else {
        char *response = "200|minute_id=0;msg=No_minutes\r\n";
        send_response(socket_fd, response);
    }
    sqlite3_finalize(stmt);

cleanup:
    if (token) free(token);
    if (meeting_id_str) free(meeting_id_str);
}

// VIEW_GROUP_DETAIL handler
void handle_view_group_detail(int socket_fd, char *payload) {
    char *token = get_value(payload, "token");
    char *group_id_str = get_value(payload, "group_id");

    if (!token || !group_id_str) {
        char *response = "400|msg=Missing_fields\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    int group_id = atoi(group_id_str);

    // Get group info
    const char *grp_sql = "SELECT group_name FROM groups WHERE group_id = ?";
    sqlite3_stmt *grp_stmt;
    sqlite3_prepare_v2(db, grp_sql, -1, &grp_stmt, NULL);
    sqlite3_bind_int(grp_stmt, 1, group_id);

    if (sqlite3_step(grp_stmt) != SQLITE_ROW) {
        char *response = "404|msg=Group_not_found\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(grp_stmt);
        goto cleanup;
    }

    char group_name[128];
    strncpy(group_name, (const char *)sqlite3_column_text(grp_stmt, 0), 127);
    sqlite3_finalize(grp_stmt);

    // Get members
    const char *sql = "SELECT u.user_id, u.full_name, gm.role FROM group_members gm "
                      "JOIN users u ON gm.user_id = u.user_id WHERE gm.group_id = ?";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, group_id);

    char members[2048] = "";
    int first = 1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) strcat(members, "#");
        char item[128];
        snprintf(item, sizeof(item), "%d,%s,%d",
            sqlite3_column_int(stmt, 0),
            sqlite3_column_text(stmt, 1),
            sqlite3_column_int(stmt, 2));
        strcat(members, item);
        first = 0;
    }
    sqlite3_finalize(stmt);

    char response[4096];
    snprintf(response, sizeof(response), "200|group_id=%d;group_name=%s;members=%s\r\n", group_id, group_name, members);
    send_response(socket_fd, response);

cleanup:
    if (token) free(token);
    if (group_id_str) free(group_id_str);
}

// REQUEST_JOIN_GROUP handler
void handle_request_join_group(int socket_fd, char *payload) {
    char *token = get_value(payload, "token");
    char *group_id_str = get_value(payload, "group_id");
    char *note = get_value(payload, "note");

    int user_id = 0;
    if (token) sscanf(token, "token_%d", &user_id);

    if (!token || !group_id_str || user_id == 0) {
        char *response = "400|msg=Missing_fields\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    int group_id = atoi(group_id_str);

    // Check if already member
    const char *member_sql = "SELECT 1 FROM group_members WHERE group_id = ? AND user_id = ?";
    sqlite3_stmt *member_stmt;
    sqlite3_prepare_v2(db, member_sql, -1, &member_stmt, NULL);
    sqlite3_bind_int(member_stmt, 1, group_id);
    sqlite3_bind_int(member_stmt, 2, user_id);

    if (sqlite3_step(member_stmt) == SQLITE_ROW) {
        char *response = "409|msg=Already_member\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(member_stmt);
        goto cleanup;
    }
    sqlite3_finalize(member_stmt);

    // Check if request already exists
    const char *exist_sql = "SELECT 1 FROM join_requests WHERE group_id = ? AND user_id = ? AND status = 'PENDING'";
    sqlite3_stmt *exist_stmt;
    sqlite3_prepare_v2(db, exist_sql, -1, &exist_stmt, NULL);
    sqlite3_bind_int(exist_stmt, 1, group_id);
    sqlite3_bind_int(exist_stmt, 2, user_id);

    if (sqlite3_step(exist_stmt) == SQLITE_ROW) {
        char *response = "409|msg=Request_already_exists\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(exist_stmt);
        goto cleanup;
    }
    sqlite3_finalize(exist_stmt);

    const char *sql = "INSERT INTO join_requests (group_id, user_id, note, status) VALUES (?, ?, ?, 'PENDING')";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_bind_text(stmt, 3, note ? note : "", -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        int request_id = (int)sqlite3_last_insert_rowid(db);
        char response[256];
        snprintf(response, sizeof(response), "202|request_id=%d;msg=Request_pending\r\n", request_id);
        send_response(socket_fd, response);
        
        // Get requester name and group name for notification
        char requester_name[128] = "";
        char group_name[128] = "";
        
        const char *name_sql = "SELECT full_name FROM users WHERE user_id = ?";
        sqlite3_stmt *name_stmt;
        sqlite3_prepare_v2(db, name_sql, -1, &name_stmt, NULL);
        sqlite3_bind_int(name_stmt, 1, user_id);
        if (sqlite3_step(name_stmt) == SQLITE_ROW) {
            strncpy(requester_name, (const char *)sqlite3_column_text(name_stmt, 0), 127);
        }
        sqlite3_finalize(name_stmt);
        
        const char *grp_sql = "SELECT group_name FROM groups WHERE group_id = ?";
        sqlite3_stmt *grp_stmt;
        sqlite3_prepare_v2(db, grp_sql, -1, &grp_stmt, NULL);
        sqlite3_bind_int(grp_stmt, 1, group_id);
        if (sqlite3_step(grp_stmt) == SQLITE_ROW) {
            strncpy(group_name, (const char *)sqlite3_column_text(grp_stmt, 0), 127);
        }
        sqlite3_finalize(grp_stmt);
        
        // Notify group admins about new join request
        const char *admin_sql = "SELECT user_id FROM group_members WHERE group_id = ? AND role = 1";
        sqlite3_stmt *admin_stmt;
        sqlite3_prepare_v2(db, admin_sql, -1, &admin_stmt, NULL);
        sqlite3_bind_int(admin_stmt, 1, group_id);
        
        while (sqlite3_step(admin_stmt) == SQLITE_ROW) {
            int admin_id = sqlite3_column_int(admin_stmt, 0);
            char noti_payload[512];
            snprintf(noti_payload, sizeof(noti_payload), 
                     "request_id=%d;group_id=%d;user_id=%d;user_name=%s;group_name=%s;note=%s", 
                     request_id, group_id, user_id, requester_name, group_name, note ? note : "");
            send_notification(admin_id, "NEW_JOIN_REQUEST", noti_payload);
        }
        sqlite3_finalize(admin_stmt);
    } else {
        char *response = "500|msg=Request_Failed\r\n";
        send_response(socket_fd, response);
    }
    sqlite3_finalize(stmt);

cleanup:
    if (token) free(token);
    if (group_id_str) free(group_id_str);
    if (note) free(note);
}

// VIEW_JOIN_REQUESTS handler
void handle_view_join_requests(int socket_fd, char *payload) {
    char *token = get_value(payload, "token");
    char *group_id_str = get_value(payload, "group_id");

    int user_id = 0;
    if (token) sscanf(token, "token_%d", &user_id);

    if (!token || !group_id_str || user_id == 0) {
        char *response = "400|msg=Missing_fields\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    int group_id = atoi(group_id_str);

    // Check if user is admin
    const char *admin_sql = "SELECT role FROM group_members WHERE group_id = ? AND user_id = ?";
    sqlite3_stmt *admin_stmt;
    sqlite3_prepare_v2(db, admin_sql, -1, &admin_stmt, NULL);
    sqlite3_bind_int(admin_stmt, 1, group_id);
    sqlite3_bind_int(admin_stmt, 2, user_id);

    if (sqlite3_step(admin_stmt) != SQLITE_ROW || sqlite3_column_int(admin_stmt, 0) != 1) {
        char *response = "403|msg=Not_group_admin\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(admin_stmt);
        goto cleanup;
    }
    sqlite3_finalize(admin_stmt);

    const char *sql = "SELECT jr.request_id, jr.user_id, u.full_name, jr.note, jr.status "
                      "FROM join_requests jr JOIN users u ON jr.user_id = u.user_id "
                      "WHERE jr.group_id = ? AND jr.status = 'PENDING'";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, group_id);

    char buffer[4096];
    snprintf(buffer, sizeof(buffer), "200|group_id=%d;requests=", group_id);
    int first = 1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) strcat(buffer, "#");
        char item[256];
        snprintf(item, sizeof(item), "%d,%d,%s,%s,%s",
            sqlite3_column_int(stmt, 0),
            sqlite3_column_int(stmt, 1),
            (const char *)sqlite3_column_text(stmt, 2),
            sqlite3_column_text(stmt, 3) ? (const char *)sqlite3_column_text(stmt, 3) : "",
            (const char *)sqlite3_column_text(stmt, 4));
        strcat(buffer, item);
        first = 0;
    }
    strcat(buffer, "\r\n");
    send_response(socket_fd, buffer);
    sqlite3_finalize(stmt);

cleanup:
    if (token) free(token);
    if (group_id_str) free(group_id_str);
}

// APPROVE_JOIN_REQUEST handler
void handle_approve_join_request(int socket_fd, char *payload) {
    char *token = get_value(payload, "token");
    char *request_id_str = get_value(payload, "request_id");

    int user_id = 0;
    if (token) sscanf(token, "token_%d", &user_id);

    if (!token || !request_id_str || user_id == 0) {
        char *response = "400|msg=Missing_fields\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    int request_id = atoi(request_id_str);

    // Get request info
    const char *req_sql = "SELECT group_id, user_id FROM join_requests WHERE request_id = ? AND status = 'PENDING'";
    sqlite3_stmt *req_stmt;
    sqlite3_prepare_v2(db, req_sql, -1, &req_stmt, NULL);
    sqlite3_bind_int(req_stmt, 1, request_id);

    if (sqlite3_step(req_stmt) != SQLITE_ROW) {
        char *response = "404|msg=Request_not_found\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(req_stmt);
        goto cleanup;
    }

    int group_id = sqlite3_column_int(req_stmt, 0);
    int requester_id = sqlite3_column_int(req_stmt, 1);
    sqlite3_finalize(req_stmt);

    // Check if user is admin
    const char *admin_sql = "SELECT role FROM group_members WHERE group_id = ? AND user_id = ?";
    sqlite3_stmt *admin_stmt;
    sqlite3_prepare_v2(db, admin_sql, -1, &admin_stmt, NULL);
    sqlite3_bind_int(admin_stmt, 1, group_id);
    sqlite3_bind_int(admin_stmt, 2, user_id);

    if (sqlite3_step(admin_stmt) != SQLITE_ROW || sqlite3_column_int(admin_stmt, 0) != 1) {
        char *response = "403|msg=Not_group_admin\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(admin_stmt);
        goto cleanup;
    }
    sqlite3_finalize(admin_stmt);

    // Add user to group
    const char *add_sql = "INSERT INTO group_members (group_id, user_id, role) VALUES (?, ?, 0)";
    sqlite3_stmt *add_stmt;
    sqlite3_prepare_v2(db, add_sql, -1, &add_stmt, NULL);
    sqlite3_bind_int(add_stmt, 1, group_id);
    sqlite3_bind_int(add_stmt, 2, requester_id);
    sqlite3_step(add_stmt);
    sqlite3_finalize(add_stmt);

    // Update request status
    const char *update_sql = "UPDATE join_requests SET status = 'APPROVED' WHERE request_id = ?";
    sqlite3_stmt *update_stmt;
    sqlite3_prepare_v2(db, update_sql, -1, &update_stmt, NULL);
    sqlite3_bind_int(update_stmt, 1, request_id);
    sqlite3_step(update_stmt);
    sqlite3_finalize(update_stmt);

    char response[256];
    snprintf(response, sizeof(response), "200|request_id=%d;group_id=%d;msg=Approved\r\n", request_id, group_id);
    send_response(socket_fd, response);
    
    // Notify the requester
    char noti_payload[256];
    snprintf(noti_payload, sizeof(noti_payload), "group_id=%d;request_id=%d", group_id, request_id);
    send_notification(requester_id, "GROUP_APPROVED", noti_payload);

cleanup:
    if (token) free(token);
    if (request_id_str) free(request_id_str);
}

// REJECT_JOIN_REQUEST handler
void handle_reject_join_request(int socket_fd, char *payload) {
    char *token = get_value(payload, "token");
    char *request_id_str = get_value(payload, "request_id");

    int user_id = 0;
    if (token) sscanf(token, "token_%d", &user_id);

    if (!token || !request_id_str || user_id == 0) {
        char *response = "400|msg=Missing_fields\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    int request_id = atoi(request_id_str);

    // Get request info
    const char *req_sql = "SELECT group_id, user_id FROM join_requests WHERE request_id = ? AND status = 'PENDING'";
    sqlite3_stmt *req_stmt;
    sqlite3_prepare_v2(db, req_sql, -1, &req_stmt, NULL);
    sqlite3_bind_int(req_stmt, 1, request_id);

    if (sqlite3_step(req_stmt) != SQLITE_ROW) {
        char *response = "404|msg=Request_not_found\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(req_stmt);
        goto cleanup;
    }

    int group_id = sqlite3_column_int(req_stmt, 0);
    int requester_id = sqlite3_column_int(req_stmt, 1);
    sqlite3_finalize(req_stmt);

    // Check if user is admin
    const char *admin_sql = "SELECT role FROM group_members WHERE group_id = ? AND user_id = ?";
    sqlite3_stmt *admin_stmt;
    sqlite3_prepare_v2(db, admin_sql, -1, &admin_stmt, NULL);
    sqlite3_bind_int(admin_stmt, 1, group_id);
    sqlite3_bind_int(admin_stmt, 2, user_id);

    if (sqlite3_step(admin_stmt) != SQLITE_ROW || sqlite3_column_int(admin_stmt, 0) != 1) {
        char *response = "403|msg=Not_group_admin\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(admin_stmt);
        goto cleanup;
    }
    sqlite3_finalize(admin_stmt);

    // Update request status
    const char *update_sql = "UPDATE join_requests SET status = 'REJECTED' WHERE request_id = ?";
    sqlite3_stmt *update_stmt;
    sqlite3_prepare_v2(db, update_sql, -1, &update_stmt, NULL);
    sqlite3_bind_int(update_stmt, 1, request_id);
    sqlite3_step(update_stmt);
    sqlite3_finalize(update_stmt);

    char response[256];
    snprintf(response, sizeof(response), "200|request_id=%d;group_id=%d;msg=Rejected\r\n", request_id, group_id);
    send_response(socket_fd, response);
    
    // Notify the requester
    char noti_payload[256];
    snprintf(noti_payload, sizeof(noti_payload), "group_id=%d;request_id=%d", group_id, request_id);
    send_notification(requester_id, "GROUP_REJECTED", noti_payload);

cleanup:
    if (token) free(token);
    if (request_id_str) free(request_id_str);
}
