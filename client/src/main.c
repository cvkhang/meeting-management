#include "client.h"

int main() {
    session_t session = {0};
    
    // Connect to server
    session.socket_fd = connect_to_server();
    if (session.socket_fd < 0) {
        printf("Không thể kết nối đến server!\n");
        printf("Hãy chắc chắn server đang chạy trên %s:%d\n", SERVER_IP, SERVER_PORT);
        return 1;
    }
    
    printf("Đã kết nối đến server!\n");
    
    // Show main menu
    show_main_menu(&session);
    
    // Close connection
    close(session.socket_fd);
    
    return 0;
}
