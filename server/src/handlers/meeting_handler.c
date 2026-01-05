#include "server.h"
#include "utils/utils.h"

extern sqlite3 *db;

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

    int slot_id = atoi(slot_id_str);

    // First, get the slot time details
    const char *slot_info_sql = "SELECT date, start_time, end_time FROM slots WHERE slot_id = ?";
    sqlite3_stmt *slot_info_stmt;
    sqlite3_prepare_v2(db, slot_info_sql, -1, &slot_info_stmt, NULL);
    sqlite3_bind_int(slot_info_stmt, 1, slot_id);

    if (sqlite3_step(slot_info_stmt) != SQLITE_ROW) {
        char *response = "404|msg=Slot_not_found\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(slot_info_stmt);
        goto cleanup;
    }

    const char *slot_date = (const char *)sqlite3_column_text(slot_info_stmt, 0);
    const char *slot_start = (const char *)sqlite3_column_text(slot_info_stmt, 1);
    const char *slot_end = (const char *)sqlite3_column_text(slot_info_stmt, 2);

    // Check if student has conflicting meetings
    const char *conflict_sql = 
        "SELECT m.meeting_id FROM meetings m "
        "JOIN slots s ON m.slot_id = s.slot_id "
        "WHERE (m.student_id = ? OR m.group_id IN (SELECT group_id FROM group_members WHERE user_id = ?)) "
        "  AND m.status = 'BOOKED' "
        "  AND s.date = ? "
        "  AND ((s.start_time < ? AND s.end_time > ?) OR "
        "       (s.start_time < ? AND s.end_time > ?) OR "
        "       (s.start_time >= ? AND s.start_time < ?))";
    
    sqlite3_stmt *conflict_stmt;
    sqlite3_prepare_v2(db, conflict_sql, -1, &conflict_stmt, NULL);
    sqlite3_bind_int(conflict_stmt, 1, student_id);
    sqlite3_bind_int(conflict_stmt, 2, student_id);
    sqlite3_bind_text(conflict_stmt, 3, slot_date, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(conflict_stmt, 4, slot_end, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(conflict_stmt, 5, slot_start, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(conflict_stmt, 6, slot_end, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(conflict_stmt, 7, slot_start, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(conflict_stmt, 8, slot_start, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(conflict_stmt, 9, slot_end, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(conflict_stmt) == SQLITE_ROW) {
        int conflicting_meeting = sqlite3_column_int(conflict_stmt, 0);
        char response[256];
        snprintf(response, sizeof(response), "409|msg=Time_conflict;conflicting_meeting=%d\r\n", conflicting_meeting);
        send_response(socket_fd, response);
        sqlite3_finalize(conflict_stmt);
        sqlite3_finalize(slot_info_stmt);
        goto cleanup;
    }
    sqlite3_finalize(conflict_stmt);
    sqlite3_finalize(slot_info_stmt);

    // Check if slot is already booked
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

    const char *sql = "INSERT INTO meetings (slot_id, teacher_id, student_id, meeting_type, status) VALUES (?, ?, ?, 'INDIVIDUAL', 'BOOKED')";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, slot_id);
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

    int slot_id = atoi(slot_id_str);

    // Get slot time details
    const char *slot_info_sql = "SELECT date, start_time, end_time FROM slots WHERE slot_id = ?";
    sqlite3_stmt *slot_info_stmt;
    sqlite3_prepare_v2(db, slot_info_sql, -1, &slot_info_stmt, NULL);
    sqlite3_bind_int(slot_info_stmt, 1, slot_id);

    if (sqlite3_step(slot_info_stmt) != SQLITE_ROW) {
        char *response = "404|msg=Slot_not_found\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(slot_info_stmt);
        goto cleanup;
    }

    const char *slot_date = (const char *)sqlite3_column_text(slot_info_stmt, 0);
    const char *slot_start = (const char *)sqlite3_column_text(slot_info_stmt, 1);
    const char *slot_end = (const char *)sqlite3_column_text(slot_info_stmt, 2);

    // Check if ANY group member has conflicting meetings
    const char *conflict_sql = 
        "SELECT m.meeting_id FROM meetings m "
        "JOIN slots s ON m.slot_id = s.slot_id "
        "WHERE (m.student_id IN (SELECT user_id FROM group_members WHERE group_id = ?) "
        "    OR m.group_id IN (SELECT gm.group_id FROM group_members gm1 "
        "                      JOIN group_members gm ON gm1.user_id = gm.user_id "
        "                      WHERE gm1.group_id = ?)) "
        "  AND m.status = 'BOOKED' "
        "  AND s.date = ? "
        "  AND ((s.start_time < ? AND s.end_time > ?) OR "
        "       (s.start_time < ? AND s.end_time > ?) OR "
        "       (s.start_time >= ? AND s.start_time < ?))";
    
    sqlite3_stmt *conflict_stmt;
    sqlite3_prepare_v2(db, conflict_sql, -1, &conflict_stmt, NULL);
    sqlite3_bind_int(conflict_stmt, 1, group_id);
    sqlite3_bind_int(conflict_stmt, 2, group_id);
    sqlite3_bind_text(conflict_stmt, 3, slot_date, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(conflict_stmt, 4, slot_end, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(conflict_stmt, 5, slot_start, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(conflict_stmt, 6, slot_end, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(conflict_stmt, 7, slot_start, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(conflict_stmt, 8, slot_start, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(conflict_stmt, 9, slot_end, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(conflict_stmt) == SQLITE_ROW) {
        int conflicting_meeting = sqlite3_column_int(conflict_stmt, 0);
        char response[256];
        snprintf(response, sizeof(response), "409|msg=Time_conflict;conflicting_meeting=%d\r\n", conflicting_meeting);
        send_response(socket_fd, response);
        sqlite3_finalize(conflict_stmt);
        sqlite3_finalize(slot_info_stmt);
        goto cleanup;
    }
    sqlite3_finalize(conflict_stmt);
    sqlite3_finalize(slot_info_stmt);

    // Check if slot is already booked
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

    // Check group member count vs slot max_group_size
    const char *count_sql = "SELECT COUNT(*) FROM group_members WHERE group_id = ?";
    sqlite3_stmt *count_stmt;
    sqlite3_prepare_v2(db, count_sql, -1, &count_stmt, NULL);
    sqlite3_bind_int(count_stmt, 1, group_id);

    int member_count = 0;
    if (sqlite3_step(count_stmt) == SQLITE_ROW) {
        member_count = sqlite3_column_int(count_stmt, 0);
    }
    sqlite3_finalize(count_stmt);

    // Get slot max_group_size
    const char *slot_sql = "SELECT max_group_size FROM slots WHERE slot_id = ?";
    sqlite3_stmt *slot_max_stmt;
    sqlite3_prepare_v2(db, slot_sql, -1, &slot_max_stmt, NULL);
    sqlite3_bind_int(slot_max_stmt, 1, slot_id);

    int max_group_size = 0;
    if (sqlite3_step(slot_max_stmt) == SQLITE_ROW) {
        max_group_size = sqlite3_column_int(slot_max_stmt, 0);
    }
    sqlite3_finalize(slot_max_stmt);

    if (member_count > max_group_size) {
        char response[256];
        snprintf(response, sizeof(response), 
                 "400|msg=Group_too_large;member_count=%d;max_size=%d\r\n", 
                 member_count, max_group_size);
        send_response(socket_fd, response);
        goto cleanup;
    }

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
        
        // Send notifications to teacher and all group members
        char noti_payload[512];
        snprintf(noti_payload, sizeof(noti_payload), 
                 "meeting_id=%d;slot_id=%s;group_id=%d;meeting_type=GROUP",
                 meeting_id, slot_id_str, group_id);
        
        // Notify teacher
        send_notification(atoi(teacher_id_str), "MEETING_BOOKED", noti_payload);
        
        // Notify all group members
        const char *members_sql = "SELECT user_id FROM group_members WHERE group_id = ?";
        sqlite3_stmt *members_stmt;
        sqlite3_prepare_v2(db, members_sql, -1, &members_stmt, NULL);
        sqlite3_bind_int(members_stmt, 1, group_id);
        
        while (sqlite3_step(members_stmt) == SQLITE_ROW) {
            int member_id = sqlite3_column_int(members_stmt, 0);
            send_notification(member_id, "MEETING_BOOKED", noti_payload);
        }
        sqlite3_finalize(members_stmt);
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

    // For group meetings, only admin or teacher can cancel
    if (group_id > 0 && user_id != teacher_id) {
        const char *admin_sql = "SELECT role FROM group_members WHERE group_id = ? AND user_id = ?";
        sqlite3_stmt *admin_stmt;
        sqlite3_prepare_v2(db, admin_sql, -1, &admin_stmt, NULL);
        sqlite3_bind_int(admin_stmt, 1, group_id);
        sqlite3_bind_int(admin_stmt, 2, user_id);
        
        int is_admin = 0;
        if (sqlite3_step(admin_stmt) == SQLITE_ROW) {
            is_admin = sqlite3_column_int(admin_stmt, 0);
        }
        sqlite3_finalize(admin_stmt);
        
        if (!is_admin) {
            char *response = "403|msg=Only_group_admin_can_cancel\r\n";
            send_response(socket_fd, response);
            goto cleanup;
        }
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

void handle_complete_meeting(int socket_fd, char *payload) {
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

    // Check if meeting exists and user is the teacher
    const char *check_sql = "SELECT teacher_id, status FROM meetings WHERE meeting_id = ?";
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
    const char *status_ptr = (const char *)sqlite3_column_text(check_stmt, 1);
    
    // Copy status before finalizing (avoid use-after-free)
    char status[32];
    strncpy(status, status_ptr, sizeof(status) - 1);
    status[sizeof(status) - 1] = '\0';
    sqlite3_finalize(check_stmt);

    // Only teacher can complete meeting
    if (user_id != teacher_id) {
        char *response = "403|msg=Not_meeting_teacher\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

    // Only BOOKED meetings can be completed
    if (strcmp(status, "BOOKED") != 0) {
        char response[256];
        snprintf(response, sizeof(response), "400|msg=Invalid_status;current=%s\r\n", status);
        send_response(socket_fd, response);
        goto cleanup;
    }

    // Mark meeting as DONE
    const char *sql = "UPDATE meetings SET status = 'DONE' WHERE meeting_id = ?";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, meeting_id);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        char response[256];
        snprintf(response, sizeof(response), "200|meeting_id=%d;msg=Completed\r\n", meeting_id);
        send_response(socket_fd, response);
    } else {
        char *response = "500|msg=Complete_Failed\r\n";
        send_response(socket_fd, response);
    }
    sqlite3_finalize(stmt);

cleanup:
    if (token) free(token);
    if (meeting_id_str) free(meeting_id_str);
}

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
