#include "client.h"

void clear_screen() {
    printf("\033[2J\033[H");
}

void print_header(const char *title) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  %-58s║\n", title);
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
}

void print_menu_item(int num, const char *text) {
    printf("  [%d] %s\n", num, text);
}

void wait_for_enter() {
    printf("\nNhấn Enter để tiếp tục...");
    getchar();
}

void show_main_menu(session_t *session) {
    int choice;
    
    while (1) {
        clear_screen();
        print_header("HỆ THỐNG QUẢN LÝ LỊCH HẸN");
        print_menu_item(1, "Đăng nhập");
        print_menu_item(2, "Đăng ký");
        print_menu_item(0, "Thoát");
        printf("\n  Lựa chọn: ");
        
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');
        
        switch (choice) {
            case 1:
                show_login_menu(session);
                if (session->is_logged_in) {
                    if (strcmp(session->role, "STUDENT") == 0) {
                        show_student_menu(session);
                    } else {
                        show_teacher_menu(session);
                    }
                }
                break;
            case 2:
                show_register_menu(session);
                break;
            case 0:
                printf("\nTạm biệt!\n");
                return;
            default:
                printf("Lựa chọn không hợp lệ!\n");
                wait_for_enter();
        }
    }
}

void show_login_menu(session_t *session) {
    clear_screen();
    print_header("ĐĂNG NHẬP");
    
    char username[64], password[64], role[16];
    int role_choice;
    
    printf("  Vai trò:\n");
    print_menu_item(1, "Sinh viên");
    print_menu_item(2, "Giảng viên");
    printf("  Chọn vai trò: ");
    scanf("%d", &role_choice);
    while (getchar() != '\n');
    
    strcpy(role, role_choice == 1 ? "STUDENT" : "TEACHER");
    
    printf("  Tên đăng nhập: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;
    
    printf("  Mật khẩu: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = 0;
    
    char message[256];
    snprintf(message, sizeof(message), "LOGIN|role=%s;username=%s;password=%s\r\n", role, username, password);
    
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    int status = get_status_code(response);
    if (status == 200) {
        session->is_logged_in = 1;
        strcpy(session->role, role);
        char *user_id = get_value_from_response(response, "user_id");
        char *token = get_value_from_response(response, "token");
        if (user_id) {
            session->user_id = atoi(user_id);
            free(user_id);
        }
        if (token) {
            strcpy(session->token, token);
            free(token);
        }
        printf("\n  ✓ Đăng nhập thành công!\n");
    } else {
        printf("\n  ✗ Đăng nhập thất bại! Sai tên đăng nhập hoặc mật khẩu.\n");
    }
    wait_for_enter();
}

void show_register_menu(session_t *session) {
    clear_screen();
    print_header("ĐĂNG KÝ TÀI KHOẢN");
    
    char username[64], password[64], full_name[128], role[16];
    int role_choice;
    
    printf("  Vai trò:\n");
    print_menu_item(1, "Sinh viên");
    print_menu_item(2, "Giảng viên");
    printf("  Chọn vai trò: ");
    scanf("%d", &role_choice);
    while (getchar() != '\n');
    
    strcpy(role, role_choice == 1 ? "STUDENT" : "TEACHER");
    
    printf("  Tên đăng nhập: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;
    
    printf("  Mật khẩu: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = 0;
    
    printf("  Họ và tên: ");
    fgets(full_name, sizeof(full_name), stdin);
    full_name[strcspn(full_name, "\n")] = 0;
    
    char message[512];
    snprintf(message, sizeof(message), "REGISTER|role=%s;username=%s;password=%s;full_name=%s\r\n", 
             role, username, password, full_name);
    
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    int status = get_status_code(response);
    if (status == 201) {
        printf("\n  ✓ Đăng ký thành công! Vui lòng đăng nhập.\n");
    } else if (status == 409) {
        printf("\n  ✗ Tên đăng nhập đã tồn tại!\n");
    } else {
        printf("\n  ✗ Đăng ký thất bại!\n");
    }
    wait_for_enter();
}

void show_student_menu(session_t *session) {
    int choice;
    
    while (session->is_logged_in) {
        // Check for notifications
        check_for_notification(session);
        
        clear_screen();
        print_header("MENU SINH VIÊN");
        print_menu_item(1, "Xem danh sách giảng viên");
        print_menu_item(2, "Xem lịch hẹn của tôi");
        print_menu_item(3, "Đặt lịch hẹn");
        print_menu_item(4, "Hủy lịch hẹn");
        print_menu_item(5, "Xem lịch sử làm việc");
        print_menu_item(6, "Xem biên bản cuộc họp");
        print_menu_item(7, "Quản lý nhóm");
        print_menu_item(0, "Đăng xuất");
        printf("\n  Lựa chọn: ");
        
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');
        
        switch (choice) {
            case 1: student_view_teachers(session); break;
            case 2: student_view_meetings(session); break;
            case 3: student_book_meeting(session); break;
            case 4: student_cancel_meeting(session); break;
            case 5: student_view_history(session); break;
            case 6: student_view_minutes(session); break;
            case 7: student_groups_menu(session); break;
            case 0:
                session->is_logged_in = 0;
                printf("\n  Đã đăng xuất.\n");
                wait_for_enter();
                return;
            default:
                printf("Lựa chọn không hợp lệ!\n");
                wait_for_enter();
        }
    }
}

void show_teacher_menu(session_t *session) {
    int choice;
    
    while (session->is_logged_in) {
        // Check for notifications
        check_for_notification(session);
        clear_screen();
        print_header("MENU GIẢNG VIÊN");
        print_menu_item(1, "Quản lý khe thời gian");
        print_menu_item(2, "Xem lịch hẹn");
        print_menu_item(3, "Xem chi tiết cuộc họp");
        print_menu_item(4, "Ghi biên bản cuộc họp");
        print_menu_item(5, "Xem lịch sử làm việc");
        print_menu_item(6, "Quản lý nhóm sinh viên");
        print_menu_item(0, "Đăng xuất");
        printf("\n  Lựa chọn: ");
        
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');
        
        switch (choice) {
            case 1: teacher_manage_slots(session); break;
            case 2: teacher_view_meetings(session); break;
            case 3: teacher_view_meeting_detail(session); break;
            case 4: teacher_write_minutes(session); break;
            case 5: student_view_history(session); break;
            case 6: teacher_groups_menu(session); break;
            case 0:
                session->is_logged_in = 0;
                printf("\n  Đã đăng xuất.\n");
                wait_for_enter();
                return;
            default:
                printf("Lựa chọn không hợp lệ!\n");
                wait_for_enter();
        }
    }
}

// Student functions
void student_view_teachers(session_t *session) {
    clear_screen();
    print_header("DANH SÁCH GIẢNG VIÊN");
    
    char message[256];
    snprintf(message, sizeof(message), "VIEW_TEACHERS|token=%s\r\n", session->token);
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    char *teachers = get_value_from_response(response, "teachers");
    if (teachers && strlen(teachers) > 0) {
        printf("  %-6s %-30s %s\n", "ID", "Họ tên", "Slot trống");
        printf("  ─────────────────────────────────────────────\n");
        
        char *teacher = strtok(teachers, "#");
        while (teacher) {
            int id, slots;
            char name[128];
            if (sscanf(teacher, "%d,%127[^,],%d", &id, name, &slots) == 3) {
                printf("  %-6d %-30s %d\n", id, name, slots);
            }
            teacher = strtok(NULL, "#");
        }
        free(teachers);
    } else {
        printf("  Không có giảng viên nào.\n");
    }
    wait_for_enter();
}

void student_view_slots(session_t *session) {
    int teacher_id;
    printf("  Nhập ID giảng viên: ");
    scanf("%d", &teacher_id);
    while (getchar() != '\n');
    
    char message[256];
    snprintf(message, sizeof(message), "VIEW_SLOTS|teacher_id=%d\r\n", teacher_id);
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    printf("\n  %-6s %-12s %-8s %-8s %-12s\n", "ID", "Ngày", "Bắt đầu", "Kết thúc", "Loại");
    printf("  ───────────────────────────────────────────────────\n");
    
    char *slots = get_value_from_response(response, "slots");
    if (slots && strlen(slots) > 0) {
        char *slot = strtok(slots, "#");
        while (slot) {
            int id, max;
            char date[16], st[8], et[8], type[16];
            if (sscanf(slot, "%d,%15[^,],%7[^,],%7[^,],%15[^,],%d", &id, date, st, et, type, &max) >= 5) {
                printf("  %-6d %-12s %-8s %-8s %-12s\n", id, date, st, et, type);
            }
            slot = strtok(NULL, "#");
        }
        free(slots);
    } else {
        printf("  Không có slot nào.\n");
    }
}

void student_book_meeting(session_t *session) {
    clear_screen();
    print_header("ĐẶT LỊCH HẸN");
    
    student_view_teachers(session);
    student_view_slots(session);
    
    int teacher_id, slot_id;
    printf("\n  Nhập ID giảng viên: ");
    scanf("%d", &teacher_id);
    printf("  Nhập ID slot: ");
    scanf("%d", &slot_id);
    while (getchar() != '\n');
    
    char message[256];
    snprintf(message, sizeof(message), "BOOK_MEETING_INDIV|token=%s;teacher_id=%d;slot_id=%d\r\n", 
             session->token, teacher_id, slot_id);
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    int status = get_status_code(response);
    if (status == 201) {
        printf("\n  ✓ Đặt lịch thành công!\n");
    } else if (status == 409) {
        printf("\n  ✗ Slot đã được đặt!\n");
    } else {
        printf("\n  ✗ Đặt lịch thất bại!\n");
    }
    wait_for_enter();
}

void student_view_meetings(session_t *session) {
    clear_screen();
    print_header("LỊCH HẸN CỦA TÔI");
    
    char message[256];
    snprintf(message, sizeof(message), "VIEW_MEETINGS|token=%s\r\n", session->token);
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    printf("  %-6s %-12s %-8s %-8s %-12s %-10s\n", "ID", "Ngày", "Bắt đầu", "Kết thúc", "Loại", "Trạng thái");
    printf("  ────────────────────────────────────────────────────────────\n");
    
    char *meetings = get_value_from_response(response, "meetings");
    if (meetings && strlen(meetings) > 0) {
        char *meeting = strtok(meetings, "#");
        while (meeting) {
            int mid, sid, pid;
            char date[16], st[8], et[8], type[16], status_str[16];
            if (sscanf(meeting, "%d,%15[^,],%7[^,],%7[^,],%d,%d,%15[^,],%15s", 
                       &mid, date, st, et, &sid, &pid, type, status_str) >= 7) {
                printf("  %-6d %-12s %-8s %-8s %-12s %-10s\n", mid, date, st, et, type, status_str);
            }
            meeting = strtok(NULL, "#");
        }
        free(meetings);
    } else {
        printf("  Không có lịch hẹn nào.\n");
    }
    wait_for_enter();
}

void student_cancel_meeting(session_t *session) {
    clear_screen();
    print_header("HỦY LỊCH HẸN");
    
    student_view_meetings(session);
    
    int meeting_id;
    char reason[256];
    printf("\n  Nhập ID cuộc họp cần hủy: ");
    scanf("%d", &meeting_id);
    while (getchar() != '\n');
    
    printf("  Lý do hủy: ");
    fgets(reason, sizeof(reason), stdin);
    reason[strcspn(reason, "\n")] = 0;
    
    char message[512];
    snprintf(message, sizeof(message), "CANCEL_MEETING|token=%s;meeting_id=%d;reason=%s\r\n", 
             session->token, meeting_id, reason);
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    int status = get_status_code(response);
    if (status == 200) {
        printf("\n  ✓ Đã hủy lịch hẹn!\n");
    } else {
        printf("\n  ✗ Không thể hủy lịch hẹn!\n");
    }
    wait_for_enter();
}

void student_view_history(session_t *session) {
    clear_screen();
    print_header("LỊCH SỬ LÀM VIỆC");
    
    char message[256];
    snprintf(message, sizeof(message), "VIEW_MEETING_HISTORY|token=%s\r\n", session->token);
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    printf("  %-6s %-12s %-8s %-8s %-20s %-10s\n", "ID", "Ngày", "Bắt đầu", "Kết thúc", "Đối tác", "Biên bản");
    printf("  ────────────────────────────────────────────────────────────────\n");
    
    char *history = get_value_from_response(response, "history");
    if (history && strlen(history) > 0) {
        char *h = strtok(history, "#");
        while (h) {
            int mid, has_min;
            char date[16], st[8], et[8], name[64], type[16];
            if (sscanf(h, "%d,%15[^,],%7[^,],%7[^,],%63[^,],%15[^,],%d", 
                       &mid, date, st, et, name, type, &has_min) >= 6) {
                printf("  %-6d %-12s %-8s %-8s %-20s %s\n", mid, date, st, et, name, has_min ? "Có" : "Không");
            }
            h = strtok(NULL, "#");
        }
        free(history);
    } else {
        printf("  Chưa có lịch sử làm việc.\n");
    }
    wait_for_enter();
}

void student_view_minutes(session_t *session) {
    clear_screen();
    print_header("XEM BIÊN BẢN CUỘC HỌP");
    
    int meeting_id;
    printf("  Nhập ID cuộc họp: ");
    scanf("%d", &meeting_id);
    while (getchar() != '\n');
    
    char message[256];
    snprintf(message, sizeof(message), "VIEW_MINUTES|token=%s;meeting_id=%d\r\n", session->token, meeting_id);
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    char *content = get_value_from_response(response, "content");
    char *minute_id = get_value_from_response(response, "minute_id");
    
    if (minute_id && strcmp(minute_id, "0") == 0) {
        printf("\n  Chưa có biên bản cho cuộc họp này.\n");
    } else if (content) {
        printf("\n  ─────────────────────────────────────────────\n");
        printf("  %s\n", content);
        printf("  ─────────────────────────────────────────────\n");
    }
    
    if (content) free(content);
    if (minute_id) free(minute_id);
    wait_for_enter();
}

void student_groups_menu(session_t *session) {
    int choice;
    
    while (1) {
        clear_screen();
        print_header("QUẢN LÝ NHÓM");
        print_menu_item(1, "Xem nhóm của tôi");
        print_menu_item(2, "Tạo nhóm mới");
        print_menu_item(3, "Xem chi tiết nhóm");
        print_menu_item(4, "Yêu cầu vào nhóm");
        print_menu_item(5, "Xem yêu cầu vào nhóm (Admin)");
        print_menu_item(6, "Duyệt yêu cầu (Admin)");
        print_menu_item(7, "Từ chối yêu cầu (Admin)");
        print_menu_item(0, "Quay lại");
        printf("\n  Lựa chọn: ");
        
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');
        
        switch (choice) {
            case 1: view_my_groups(session); break;
            case 2: create_group(session); break;
            case 3: view_group_detail(session); break;
            case 4: request_join_group(session); break;
            case 5: view_join_requests(session); break;
            case 6: approve_reject_request(session, 1); break;
            case 7: approve_reject_request(session, 0); break;
            case 0: return;
            default:
                printf("Lựa chọn không hợp lệ!\n");
                wait_for_enter();
        }
    }
}

// Teacher functions
void teacher_manage_slots(session_t *session) {
    int choice;
    
    while (1) {
        clear_screen();
        print_header("QUẢN LÝ KHE THỜI GIAN");
        print_menu_item(1, "Xem các slot đã tạo");
        print_menu_item(2, "Tạo slot mới");
        print_menu_item(3, "Sửa slot");
        print_menu_item(4, "Xóa slot");
        print_menu_item(0, "Quay lại");
        printf("\n  Lựa chọn: ");
        
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');
        
        switch (choice) {
            case 1: {
                char message[256];
                snprintf(message, sizeof(message), "VIEW_SLOTS|teacher_id=%d\r\n", session->user_id);
                send_message(session->socket_fd, message);
                
                char response[BUFFER_SIZE];
                receive_message(session->socket_fd, response, BUFFER_SIZE);
                
                printf("\n  %-6s %-12s %-8s %-8s %-12s %-6s\n", "ID", "Ngày", "Bắt đầu", "Kết thúc", "Loại", "Max");
                printf("  ─────────────────────────────────────────────────────────\n");
                
                char *slots = get_value_from_response(response, "slots");
                if (slots && strlen(slots) > 0) {
                    char *slot = strtok(slots, "#");
                    while (slot) {
                        int id, max;
                        char date[16], st[8], et[8], type[16];
                        if (sscanf(slot, "%d,%15[^,],%7[^,],%7[^,],%15[^,],%d", &id, date, st, et, type, &max) >= 5) {
                            printf("  %-6d %-12s %-8s %-8s %-12s %-6d\n", id, date, st, et, type, max);
                        }
                        slot = strtok(NULL, "#");
                    }
                    free(slots);
                }
                wait_for_enter();
                break;
            }
            case 2: teacher_create_slot(session); break;
            case 3: teacher_edit_slot(session); break;
            case 4: teacher_delete_slot(session); break;
            case 0: return;
        }
    }
}

void teacher_create_slot(session_t *session) {
    clear_screen();
    print_header("TẠO KHE THỜI GIAN MỚI");
    
    char date[16], start_time[8], end_time[8], slot_type[16];
    int max_group_size, type_choice;
    
    printf("  Ngày (YYYY-MM-DD): ");
    fgets(date, sizeof(date), stdin);
    date[strcspn(date, "\n")] = 0;
    
    printf("  Giờ bắt đầu (HH:MM): ");
    fgets(start_time, sizeof(start_time), stdin);
    start_time[strcspn(start_time, "\n")] = 0;
    
    printf("  Giờ kết thúc (HH:MM): ");
    fgets(end_time, sizeof(end_time), stdin);
    end_time[strcspn(end_time, "\n")] = 0;
    
    printf("\n  Loại slot:\n");
    print_menu_item(1, "Cá nhân (INDIVIDUAL)");
    print_menu_item(2, "Nhóm (GROUP)");
    print_menu_item(3, "Cả hai (BOTH)");
    printf("  Chọn: ");
    scanf("%d", &type_choice);
    while (getchar() != '\n');
    
    switch (type_choice) {
        case 1: strcpy(slot_type, "INDIVIDUAL"); break;
        case 2: strcpy(slot_type, "GROUP"); break;
        default: strcpy(slot_type, "BOTH"); break;
    }
    
    printf("  Số người tối đa (cho nhóm): ");
    scanf("%d", &max_group_size);
    while (getchar() != '\n');
    
    char message[512];
    snprintf(message, sizeof(message), 
             "DECLARE_SLOT|token=%s;date=%s;start_time=%s;end_time=%s;slot_type=%s;max_group_size=%d\r\n",
             session->token, date, start_time, end_time, slot_type, max_group_size);
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    int status = get_status_code(response);
    if (status == 201) {
        printf("\n  ✓ Tạo slot thành công!\n");
    } else {
        printf("\n  ✗ Tạo slot thất bại!\n");
    }
    wait_for_enter();
}

void teacher_edit_slot(session_t *session) {
    clear_screen();
    print_header("SỬA KHE THỜI GIAN");
    
    int slot_id;
    char date[16], start_time[8], end_time[8], slot_type[16];
    int max_group_size, type_choice;
    
    printf("  Nhập ID slot cần sửa: ");
    scanf("%d", &slot_id);
    while (getchar() != '\n');
    
    printf("  Ngày mới (YYYY-MM-DD): ");
    fgets(date, sizeof(date), stdin);
    date[strcspn(date, "\n")] = 0;
    
    printf("  Giờ bắt đầu mới (HH:MM): ");
    fgets(start_time, sizeof(start_time), stdin);
    start_time[strcspn(start_time, "\n")] = 0;
    
    printf("  Giờ kết thúc mới (HH:MM): ");
    fgets(end_time, sizeof(end_time), stdin);
    end_time[strcspn(end_time, "\n")] = 0;
    
    printf("\n  Loại slot mới:\n");
    print_menu_item(1, "Cá nhân (INDIVIDUAL)");
    print_menu_item(2, "Nhóm (GROUP)");
    print_menu_item(3, "Cả hai (BOTH)");
    printf("  Chọn: ");
    scanf("%d", &type_choice);
    while (getchar() != '\n');
    
    switch (type_choice) {
        case 1: strcpy(slot_type, "INDIVIDUAL"); break;
        case 2: strcpy(slot_type, "GROUP"); break;
        default: strcpy(slot_type, "BOTH"); break;
    }
    
    printf("  Số người tối đa mới: ");
    scanf("%d", &max_group_size);
    while (getchar() != '\n');
    
    char message[512];
    snprintf(message, sizeof(message), 
             "EDIT_SLOT|token=%s;slot_id=%d;action=UPDATE;date=%s;start_time=%s;end_time=%s;slot_type=%s;max_group_size=%d\r\n",
             session->token, slot_id, date, start_time, end_time, slot_type, max_group_size);
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    int status = get_status_code(response);
    if (status == 200) {
        printf("\n  ✓ Cập nhật slot thành công!\n");
    } else {
        printf("\n  ✗ Cập nhật thất bại!\n");
    }
    wait_for_enter();
}

void teacher_delete_slot(session_t *session) {
    int slot_id;
    printf("  Nhập ID slot cần xóa: ");
    scanf("%d", &slot_id);
    while (getchar() != '\n');
    
    char message[256];
    snprintf(message, sizeof(message), "EDIT_SLOT|token=%s;slot_id=%d;action=DELETE\r\n", session->token, slot_id);
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    int status = get_status_code(response);
    if (status == 200) {
        printf("\n  ✓ Xóa slot thành công!\n");
    } else {
        printf("\n  ✗ Xóa thất bại!\n");
    }
    wait_for_enter();
}

void teacher_view_meetings(session_t *session) {
    clear_screen();
    print_header("LỊCH HẸN VỚI SINH VIÊN");
    
    char message[256];
    snprintf(message, sizeof(message), "VIEW_MEETINGS|token=%s\r\n", session->token);
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    printf("  %-6s %-12s %-8s %-8s %-12s %-10s\n", "ID", "Ngày", "Bắt đầu", "Kết thúc", "Loại", "Trạng thái");
    printf("  ────────────────────────────────────────────────────────────\n");
    
    char *meetings = get_value_from_response(response, "meetings");
    if (meetings && strlen(meetings) > 0) {
        char *meeting = strtok(meetings, "#");
        while (meeting) {
            int mid, sid, pid;
            char date[16], st[8], et[8], type[16], status_str[16];
            if (sscanf(meeting, "%d,%15[^,],%7[^,],%7[^,],%d,%d,%15[^,],%15s", 
                       &mid, date, st, et, &sid, &pid, type, status_str) >= 7) {
                printf("  %-6d %-12s %-8s %-8s %-12s %-10s\n", mid, date, st, et, type, status_str);
            }
            meeting = strtok(NULL, "#");
        }
        free(meetings);
    } else {
        printf("  Không có lịch hẹn nào.\n");
    }
    wait_for_enter();
}

void teacher_view_meeting_detail(session_t *session) {
    clear_screen();
    print_header("CHI TIẾT CUỘC HỌP");
    
    int meeting_id;
    printf("  Nhập ID cuộc họp: ");
    scanf("%d", &meeting_id);
    while (getchar() != '\n');
    
    char message[256];
    snprintf(message, sizeof(message), "VIEW_MEETING_DETAIL|token=%s;meeting_id=%d\r\n", session->token, meeting_id);
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    int status = get_status_code(response);
    if (status == 200) {
        char *date = get_value_from_response(response, "date");
        char *st = get_value_from_response(response, "start_time");
        char *et = get_value_from_response(response, "end_time");
        char *type = get_value_from_response(response, "meeting_type");
        char *mstatus = get_value_from_response(response, "status");
        char *has_min = get_value_from_response(response, "has_minutes");
        
        printf("\n  ─────────────────────────────────────────────\n");
        printf("  Ngày:        %s\n", date ? date : "N/A");
        printf("  Thời gian:   %s - %s\n", st ? st : "N/A", et ? et : "N/A");
        printf("  Loại:        %s\n", type ? type : "N/A");
        printf("  Trạng thái:  %s\n", mstatus ? mstatus : "N/A");
        printf("  Biên bản:    %s\n", (has_min && strcmp(has_min, "1") == 0) ? "Đã có" : "Chưa có");
        printf("  ─────────────────────────────────────────────\n");
        
        if (date) free(date);
        if (st) free(st);
        if (et) free(et);
        if (type) free(type);
        if (mstatus) free(mstatus);
        if (has_min) free(has_min);
    } else {
        printf("\n  ✗ Không tìm thấy cuộc họp!\n");
    }
    wait_for_enter();
}

void teacher_write_minutes(session_t *session) {
    clear_screen();
    print_header("GHI BIÊN BẢN CUỘC HỌP");
    
    int meeting_id;
    char content[2048];
    
    printf("  Nhập ID cuộc họp: ");
    scanf("%d", &meeting_id);
    while (getchar() != '\n');
    
    printf("  Nhập nội dung biên bản (Enter để kết thúc):\n  ");
    fgets(content, sizeof(content), stdin);
    content[strcspn(content, "\n")] = 0;
    
    char message[4096];
    snprintf(message, sizeof(message), "SAVE_MINUTES|token=%s;meeting_id=%d;content=%s\r\n", 
             session->token, meeting_id, content);
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    int status = get_status_code(response);
    if (status == 201) {
        printf("\n  ✓ Lưu biên bản thành công!\n");
    } else if (status == 409) {
        printf("\n  Biên bản đã tồn tại. Bạn có muốn cập nhật không? (y/n): ");
        char choice;
        scanf(" %c", &choice);
        while (getchar() != '\n');
        
        if (choice == 'y' || choice == 'Y') {
            // Get minute_id first
            snprintf(message, sizeof(message), "VIEW_MINUTES|token=%s;meeting_id=%d\r\n", session->token, meeting_id);
            send_message(session->socket_fd, message);
            receive_message(session->socket_fd, response, BUFFER_SIZE);
            
            char *minute_id = get_value_from_response(response, "minute_id");
            if (minute_id) {
                snprintf(message, sizeof(message), "UPDATE_MINUTES|token=%s;minute_id=%s;content=%s\r\n", 
                         session->token, minute_id, content);
                send_message(session->socket_fd, message);
                receive_message(session->socket_fd, response, BUFFER_SIZE);
                
                if (get_status_code(response) == 200) {
                    printf("\n  ✓ Cập nhật biên bản thành công!\n");
                }
                free(minute_id);
            }
        }
    } else {
        printf("\n  ✗ Lưu biên bản thất bại!\n");
    }
    wait_for_enter();
}

void teacher_groups_menu(session_t *session) {
    student_groups_menu(session);
}

// Group functions
void view_my_groups(session_t *session) {
    clear_screen();
    print_header("NHÓM CỦA TÔI");
    
    char message[256];
    snprintf(message, sizeof(message), "VIEW_GROUPS|token=%s\r\n", session->token);
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    printf("  %-6s %-30s %-10s %-10s\n", "ID", "Tên nhóm", "Thành viên", "Vai trò");
    printf("  ───────────────────────────────────────────────────────\n");
    
    char *groups = get_value_from_response(response, "groups");
    if (groups && strlen(groups) > 0) {
        char *group = strtok(groups, "#");
        while (group) {
            int gid, count, is_admin;
            char name[128];
            if (sscanf(group, "%d,%127[^,],%d,%d", &gid, name, &count, &is_admin) >= 3) {
                printf("  %-6d %-30s %-10d %-10s\n", gid, name, count, is_admin ? "Admin" : "Member");
            }
            group = strtok(NULL, "#");
        }
        free(groups);
    } else {
        printf("  Bạn chưa tham gia nhóm nào.\n");
    }
    wait_for_enter();
}

void create_group(session_t *session) {
    clear_screen();
    print_header("TẠO NHÓM MỚI");
    
    char group_name[128];
    printf("  Tên nhóm: ");
    fgets(group_name, sizeof(group_name), stdin);
    group_name[strcspn(group_name, "\n")] = 0;
    
    char message[256];
    snprintf(message, sizeof(message), "CREATE_GROUP|token=%s;group_name=%s\r\n", session->token, group_name);
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    int status = get_status_code(response);
    if (status == 201) {
        printf("\n  ✓ Tạo nhóm thành công!\n");
    } else {
        printf("\n  ✗ Tạo nhóm thất bại!\n");
    }
    wait_for_enter();
}

void view_group_detail(session_t *session) {
    clear_screen();
    print_header("CHI TIẾT NHÓM");
    
    int group_id;
    printf("  Nhập ID nhóm: ");
    scanf("%d", &group_id);
    while (getchar() != '\n');
    
    char message[256];
    snprintf(message, sizeof(message), "VIEW_GROUP_DETAIL|token=%s;group_id=%d\r\n", session->token, group_id);
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    int status = get_status_code(response);
    if (status == 200) {
        char *name = get_value_from_response(response, "group_name");
        char *members = get_value_from_response(response, "members");
        
        printf("\n  Tên nhóm: %s\n\n", name ? name : "N/A");
        printf("  %-6s %-30s %-10s\n", "ID", "Họ tên", "Vai trò");
        printf("  ───────────────────────────────────────────\n");
        
        if (members && strlen(members) > 0) {
            char *member = strtok(members, "#");
            while (member) {
                int uid, role;
                char fname[64];
                if (sscanf(member, "%d,%63[^,],%d", &uid, fname, &role) >= 2) {
                    printf("  %-6d %-30s %-10s\n", uid, fname, role == 1 ? "Admin" : "Member");
                }
                member = strtok(NULL, "#");
            }
        }
        
        if (name) free(name);
        if (members) free(members);
    } else {
        printf("\n  ✗ Không tìm thấy nhóm!\n");
    }
    wait_for_enter();
}

void request_join_group(session_t *session) {
    clear_screen();
    print_header("YÊU CẦU VÀO NHÓM");
    
    view_my_groups(session);
    
    int group_id;
    char note[256];
    
    printf("\n  Nhập ID nhóm muốn tham gia: ");
    scanf("%d", &group_id);
    while (getchar() != '\n');
    
    printf("  Ghi chú (tùy chọn): ");
    fgets(note, sizeof(note), stdin);
    note[strcspn(note, "\n")] = 0;
    
    char message[512];
    snprintf(message, sizeof(message), "REQUEST_JOIN_GROUP|token=%s;group_id=%d;note=%s\r\n", 
             session->token, group_id, note);
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    int status = get_status_code(response);
    if (status == 202) {
        printf("\n  ✓ Đã gửi yêu cầu! Đang chờ duyệt.\n");
    } else if (status == 409) {
        char *msg = get_value_from_response(response, "msg");
        if (msg && strcmp(msg, "Already_member") == 0) {
            printf("\n  Bạn đã là thành viên của nhóm này!\n");
        } else {
            printf("\n  Yêu cầu đã tồn tại!\n");
        }
        if (msg) free(msg);
    } else {
        printf("\n  ✗ Gửi yêu cầu thất bại!\n");
    }
    wait_for_enter();
}

void view_join_requests(session_t *session) {
    clear_screen();
    print_header("YÊU CẦU VÀO NHÓM");
    
    int group_id;
    printf("  Nhập ID nhóm: ");
    scanf("%d", &group_id);
    while (getchar() != '\n');
    
    char message[256];
    snprintf(message, sizeof(message), "VIEW_JOIN_REQUESTS|token=%s;group_id=%d\r\n", session->token, group_id);
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    int status = get_status_code(response);
    if (status == 200) {
        printf("\n  %-6s %-6s %-30s %-20s %-10s\n", "ReqID", "UID", "Họ tên", "Ghi chú", "Trạng thái");
        printf("  ─────────────────────────────────────────────────────────────────\n");
        
        char *requests = get_value_from_response(response, "requests");
        if (requests && strlen(requests) > 0) {
            char *req = strtok(requests, "#");
            while (req) {
                int rid, uid;
                char name[64], note[64], req_status[16];
                if (sscanf(req, "%d,%d,%63[^,],%63[^,],%15s", &rid, &uid, name, note, req_status) >= 4) {
                    printf("  %-6d %-6d %-30s %-20s %-10s\n", rid, uid, name, note, req_status);
                }
                req = strtok(NULL, "#");
            }
            free(requests);
        } else {
            printf("  Không có yêu cầu nào.\n");
        }
    } else {
        printf("\n  ✗ Bạn không phải admin của nhóm này!\n");
    }
    wait_for_enter();
}

void approve_reject_request(session_t *session, int approve) {
    clear_screen();
    print_header(approve ? "DUYỆT YÊU CẦU" : "TỪ CHỐI YÊU CẦU");
    
    int request_id;
    printf("  Nhập ID yêu cầu: ");
    scanf("%d", &request_id);
    while (getchar() != '\n');
    
    char message[256];
    snprintf(message, sizeof(message), "%s|token=%s;request_id=%d\r\n", 
             approve ? "APPROVE_JOIN_REQUEST" : "REJECT_JOIN_REQUEST", session->token, request_id);
    send_message(session->socket_fd, message);
    
    char response[BUFFER_SIZE];
    receive_message(session->socket_fd, response, BUFFER_SIZE);
    
    int status = get_status_code(response);
    if (status == 200) {
        printf("\n  ✓ %s thành công!\n", approve ? "Duyệt" : "Từ chối");
    } else {
        printf("\n  ✗ Thao tác thất bại!\n");
    }
    wait_for_enter();
}
