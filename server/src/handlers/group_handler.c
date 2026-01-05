#include "server.h"
#include "utils/utils.h"

extern sqlite3 *db;

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

    // Check if group exists
    const char *grp_exist_sql = "SELECT group_id FROM groups WHERE group_id = ?";
    sqlite3_stmt *grp_exist_stmt;
    sqlite3_prepare_v2(db, grp_exist_sql, -1, &grp_exist_stmt, NULL);
    sqlite3_bind_int(grp_exist_stmt, 1, group_id);

    if (sqlite3_step(grp_exist_stmt) != SQLITE_ROW) {
        char *response = "404|msg=Group_not_found\r\n";
        send_response(socket_fd, response);
        sqlite3_finalize(grp_exist_stmt);
        goto cleanup;
    }
    sqlite3_finalize(grp_exist_stmt);

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
