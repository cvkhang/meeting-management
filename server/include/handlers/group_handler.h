#ifndef GROUP_HANDLER_H
#define GROUP_HANDLER_H

// Group management handlers
void handle_create_group(int socket_fd, char *payload);
void handle_view_groups(int socket_fd, char *payload);
void handle_view_group_detail(int socket_fd, char *payload);
void handle_request_join_group(int socket_fd, char *payload);
void handle_view_join_requests(int socket_fd, char *payload);
void handle_approve_join_request(int socket_fd, char *payload);
void handle_reject_join_request(int socket_fd, char *payload);

#endif // GROUP_HANDLER_H
