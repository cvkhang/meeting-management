#include "server.h"
#include "utils/utils.h"

extern sqlite3 *db;

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
    
    // Enforce slot type constraints
    int max_group_size;
    if (strcmp(slot_type, "INDIVIDUAL") == 0) {
        max_group_size = 1;  // Force 1 for INDIVIDUAL slots
    } else if (strcmp(slot_type, "GROUP") == 0) {
        max_group_size = max_group_size_str ? atoi(max_group_size_str) : 5;
    } else {  // BOTH
        max_group_size = max_group_size_str ? atoi(max_group_size_str) : 5;
    }

    // Check for time overlap with existing slots
    const char *overlap_sql = 
        "SELECT slot_id FROM slots "
        "WHERE teacher_id = ? AND date = ? "
        "AND ((start_time < ? AND end_time > ?) OR "  // New slot inside existing
        "     (start_time < ? AND end_time > ?) OR "  // Overlaps at start
        "     (start_time >= ? AND start_time < ?))"; // Overlaps at end
    
    sqlite3_stmt *overlap_stmt;
    sqlite3_prepare_v2(db, overlap_sql, -1, &overlap_stmt, NULL);
    sqlite3_bind_int(overlap_stmt, 1, teacher_id);
    sqlite3_bind_text(overlap_stmt, 2, date, -1, SQLITE_STATIC);
    sqlite3_bind_text(overlap_stmt, 3, end_time, -1, SQLITE_STATIC);
    sqlite3_bind_text(overlap_stmt, 4, start_time, -1, SQLITE_STATIC);
    sqlite3_bind_text(overlap_stmt, 5, end_time, -1, SQLITE_STATIC);
    sqlite3_bind_text(overlap_stmt, 6, start_time, -1, SQLITE_STATIC);
    sqlite3_bind_text(overlap_stmt, 7, start_time, -1, SQLITE_STATIC);
    sqlite3_bind_text(overlap_stmt, 8, end_time, -1, SQLITE_STATIC);
    
    if (sqlite3_step(overlap_stmt) == SQLITE_ROW) {
        int conflicting_slot = sqlite3_column_int(overlap_stmt, 0);
        char response[256];
        snprintf(response, sizeof(response), "409|msg=Time_conflict;conflicting_slot=%d\r\n", conflicting_slot);
        send_response(socket_fd, response);
        sqlite3_finalize(overlap_stmt);
        goto cleanup;
    }
    sqlite3_finalize(overlap_stmt);

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
    
    if (!teacher_id_str) {
        char *response = "400|msg=Missing_teacher_id\r\n";
        send_response(socket_fd, response);
        return;
    }

    // Show slots that don't have DONE meetings (past meetings don't show)
    // AVAILABLE: No meeting OR only CANCELLED meetings
    // BOOKED: Has BOOKED meeting
    const char *sql = 
        "SELECT s.slot_id, s.date, s.start_time, s.end_time, s.slot_type, s.max_group_size, "
        "CASE "
        "  WHEN EXISTS(SELECT 1 FROM meetings WHERE slot_id = s.slot_id AND status = 'DONE') THEN 'DONE' "
        "  WHEN EXISTS(SELECT 1 FROM meetings WHERE slot_id = s.slot_id AND status = 'BOOKED') THEN 'BOOKED' "
        "  ELSE 'AVAILABLE' "
        "END as status "
        "FROM slots s "
        "WHERE s.teacher_id = ? "
        "AND NOT EXISTS(SELECT 1 FROM meetings WHERE slot_id = s.slot_id AND status = 'DONE')";
    
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
        const unsigned char *status = sqlite3_column_text(stmt, 6);
        
        char item[256];
        snprintf(item, sizeof(item), "%d,%s,%s,%s,%s,%d,%s", sid, date, st, et, type, max, status);
        strcat(buffer, item);
        first = 0;
    }
    strcat(buffer, "\r\n");
    send_response(socket_fd, buffer);
    sqlite3_finalize(stmt);
    free(teacher_id_str);
}

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

    // Check if slot has any active bookings
    const char *booking_sql = "SELECT COUNT(*) FROM meetings WHERE slot_id = ? AND status = 'BOOKED'";
    sqlite3_stmt *booking_stmt;
    sqlite3_prepare_v2(db, booking_sql, -1, &booking_stmt, NULL);
    sqlite3_bind_int(booking_stmt, 1, slot_id);

    int booking_count = 0;
    if (sqlite3_step(booking_stmt) == SQLITE_ROW) {
        booking_count = sqlite3_column_int(booking_stmt, 0);
    }
    sqlite3_finalize(booking_stmt);

    if (booking_count > 0) {
        char *response = "403|msg=Cannot_modify_booked_slot\r\n";
        send_response(socket_fd, response);
        goto cleanup;
    }

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
            // Check for time overlap with OTHER slots (exclude current slot_id)
            const char *overlap_sql = 
                "SELECT slot_id FROM slots "
                "WHERE teacher_id = ? AND date = ? AND slot_id != ? "
                "AND ((start_time < ? AND end_time > ?) OR "
                "     (start_time < ? AND end_time > ?) OR "
                "     (start_time >= ? AND start_time < ?))";
            
            sqlite3_stmt *overlap_stmt;
            sqlite3_prepare_v2(db, overlap_sql, -1, &overlap_stmt, NULL);
            sqlite3_bind_int(overlap_stmt, 1, teacher_id);
            sqlite3_bind_text(overlap_stmt, 2, date, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(overlap_stmt, 3, slot_id);  // Exclude current slot
            sqlite3_bind_text(overlap_stmt, 4, end_time, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(overlap_stmt, 5, start_time, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(overlap_stmt, 6, end_time, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(overlap_stmt, 7, start_time, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(overlap_stmt, 8, start_time, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(overlap_stmt, 9, end_time, -1, SQLITE_TRANSIENT);
            
            if (sqlite3_step(overlap_stmt) == SQLITE_ROW) {
                int conflicting_slot_id = sqlite3_column_int(overlap_stmt, 0);
                char response[256];
                snprintf(response, sizeof(response), 
                         "409|msg=Time_conflict;conflicting_slot=%d\r\n", 
                         conflicting_slot_id);
                send_response(socket_fd, response);
                sqlite3_finalize(overlap_stmt);
                goto cleanup_update;
            }
            sqlite3_finalize(overlap_stmt);
            
            // Enforce slot type constraints
            int max_group_size;
            if (strcmp(slot_type, "INDIVIDUAL") == 0) {
                max_group_size = 1;  // Force 1 for INDIVIDUAL slots
            } else if (strcmp(slot_type, "GROUP") == 0) {
                max_group_size = max_group_size_str ? atoi(max_group_size_str) : 5;
            } else {  // BOTH
                max_group_size = max_group_size_str ? atoi(max_group_size_str) : 5;
            }
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
        
cleanup_update:
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

void handle_view_teachers(int socket_fd, char *payload) {
    // Count slots that are AVAILABLE (no BOOKED meeting, no DONE meeting)
    const char *sql = 
        "SELECT u.user_id, u.full_name, "
        "(SELECT COUNT(*) FROM slots s "
        " WHERE s.teacher_id = u.user_id "
        " AND NOT EXISTS(SELECT 1 FROM meetings WHERE slot_id = s.slot_id AND status = 'BOOKED') "
        " AND NOT EXISTS(SELECT 1 FROM meetings WHERE slot_id = s.slot_id AND status = 'DONE')"
        ") as slot_count "
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
