#include "server.h"
#include <time.h>

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
    } else if (strcmp(command, "COMPLETE_MEETING") == 0) {
        handle_complete_meeting(socket_fd, payload);
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
