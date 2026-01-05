#include "server.h"
#include "utils/utils.h"

extern sqlite3 *db;

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
