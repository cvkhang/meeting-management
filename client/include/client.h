#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1234
#define BUFFER_SIZE 4096

// Session data
typedef struct {
    int socket_fd;
    int user_id;
    char role[16];
    char token[64];
    char full_name[128];
    int is_logged_in;
} session_t;

// Network functions
int connect_to_server();
int send_message(int socket_fd, const char *message);
int receive_message(int socket_fd, char *buffer, int buffer_size);
int receive_with_timeout(int socket_fd, char *buffer, int buffer_size, int timeout_ms);
char *get_value_from_response(const char *response, const char *key);
int get_status_code(const char *response);
int check_for_notification(session_t *session);
void handle_notification(const char *notification);

// UI functions
void clear_screen();
void print_header(const char *title);
void print_menu_item(int num, const char *text);
void wait_for_enter();

void show_main_menu(session_t *session);
void show_login_menu(session_t *session);
void show_register_menu(session_t *session);
void show_student_menu(session_t *session);
void show_teacher_menu(session_t *session);

// Student UI
void student_view_teachers(session_t *session);
void student_view_slots(session_t *session);
void student_book_meeting(session_t *session);
void student_view_meetings(session_t *session);
void student_cancel_meeting(session_t *session);
void student_view_history(session_t *session);
void student_view_minutes(session_t *session);
void student_groups_menu(session_t *session);

// Teacher UI
void teacher_manage_slots(session_t *session);
void teacher_create_slot(session_t *session);
void teacher_edit_slot(session_t *session);
void teacher_delete_slot(session_t *session);
void teacher_view_meetings(session_t *session);
void teacher_view_meeting_detail(session_t *session);
void teacher_write_minutes(session_t *session);
void teacher_groups_menu(session_t *session);

// Group UI
void view_my_groups(session_t *session);
void create_group(session_t *session);
void view_group_detail(session_t *session);
void request_join_group(session_t *session);
void view_join_requests(session_t *session);
void approve_reject_request(session_t *session, int approve);

#endif // CLIENT_H
