#include "client.h"

int connect_to_server() {
    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return -1;
    }

    return sock;
}

int send_message(int socket_fd, const char *message) {
    return send(socket_fd, message, strlen(message), 0);
}

int receive_message(int socket_fd, char *buffer, int buffer_size) {
    memset(buffer, 0, buffer_size);
    int n = recv(socket_fd, buffer, buffer_size - 1, 0);
    if (n > 0) {
        buffer[n] = '\0';
    }
    return n;
}

// Receive with timeout using select()
int receive_with_timeout(int socket_fd, char *buffer, int buffer_size, int timeout_ms) {
    fd_set read_fds;
    struct timeval tv;
    
    FD_ZERO(&read_fds);
    FD_SET(socket_fd, &read_fds);
    
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    int result = select(socket_fd + 1, &read_fds, NULL, NULL, &tv);
    
    if (result > 0 && FD_ISSET(socket_fd, &read_fds)) {
        return receive_message(socket_fd, buffer, buffer_size);
    }
    
    return 0; // Timeout or no data
}

// Check for incoming notifications (non-blocking)
int check_for_notification(session_t *session) {
    if (!session->is_logged_in) return 0;
    
    char buffer[BUFFER_SIZE];
    int n = receive_with_timeout(session->socket_fd, buffer, BUFFER_SIZE, 100);
    
    if (n > 0 && strncmp(buffer, "NTF|", 4) == 0) {
        handle_notification(buffer);
        return 1;
    }
    
    return 0;
}

// Display notification to user
void handle_notification(const char *notification) {
    char *type = get_value_from_response(notification, "type");
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  🔔 THÔNG BÁO MỚI                                        ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    
    if (type) {
        if (strcmp(type, "MEETING_BOOKED") == 0) {
            char *meeting_id = get_value_from_response(notification, "meeting_id");
            printf("║  Có người đặt lịch hẹn mới! Meeting ID: %-16s ║\n", meeting_id ? meeting_id : "N/A");
            if (meeting_id) free(meeting_id);
        } else if (strcmp(type, "MEETING_CANCELLED") == 0) {
            char *meeting_id = get_value_from_response(notification, "meeting_id");
            printf("║  Lịch hẹn đã bị hủy! Meeting ID: %-22s ║\n", meeting_id ? meeting_id : "N/A");
            if (meeting_id) free(meeting_id);
        } else if (strcmp(type, "NEW_JOIN_REQUEST") == 0) {
            char *group_id = get_value_from_response(notification, "group_id");
            printf("║  Có yêu cầu gia nhập nhóm mới! Group ID: %-15s ║\n", group_id ? group_id : "N/A");
            if (group_id) free(group_id);
        } else if (strcmp(type, "GROUP_APPROVED") == 0) {
            char *group_id = get_value_from_response(notification, "group_id");
            printf("║  Yêu cầu gia nhập nhóm đã được duyệt! Group: %-11s ║\n", group_id ? group_id : "N/A");
            if (group_id) free(group_id);
        } else if (strcmp(type, "GROUP_REJECTED") == 0) {
            char *group_id = get_value_from_response(notification, "group_id");
            printf("║  Yêu cầu gia nhập nhóm bị từ chối! Group: %-13s ║\n", group_id ? group_id : "N/A");
            if (group_id) free(group_id);
        } else {
            printf("║  Loại: %-50s ║\n", type);
        }
        free(type);
    }
    
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
}

int get_status_code(const char *response) {
    int code = 0;
    sscanf(response, "%d", &code);
    return code;
}

char *get_value_from_response(const char *response, const char *key) {
    if (!response || !key) return NULL;
    
    // Find the payload part after |
    const char *payload = strchr(response, '|');
    if (!payload) return NULL;
    payload++; // Skip the |
    
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
