#ifndef MEETING_HANDLER_H
#define MEETING_HANDLER_H

// Meeting management handlers
void handle_book_meeting_indiv(int socket_fd, char *payload);
void handle_book_meeting_group(int socket_fd, char *payload);
void handle_cancel_meeting(int socket_fd, char *payload);
void handle_complete_meeting(int socket_fd, char *payload);
void handle_view_meetings(int socket_fd, char *payload);
void handle_view_meeting_detail(int socket_fd, char *payload);
void handle_view_meeting_history(int socket_fd, char *payload);

#endif // MEETING_HANDLER_H
