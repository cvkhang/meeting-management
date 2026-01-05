#ifndef SLOT_HANDLER_H
#define SLOT_HANDLER_H

// Slot management handlers
void handle_declare_slot(int socket_fd, char *payload);
void handle_edit_slot(int socket_fd, char *payload);
void handle_view_slots(int socket_fd, char *payload);
void handle_view_teachers(int socket_fd, char *payload);

#endif // SLOT_HANDLER_H
