#include "mainwindow.h"
#include "loginwidget.h"
#include "studentwidget.h"
#include "teacherwidget.h"
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_networkManager(new NetworkManager(this))
    , m_stackedWidget(new QStackedWidget(this))
    , m_loginWidget(nullptr)
    , m_studentWidget(nullptr)
    , m_teacherWidget(nullptr)
{
    setupUi();

    // Connect to server on startup
    if (!m_networkManager->connectToServer("127.0.0.1", 1234)) {
        QMessageBox::critical(this, "Lỗi", "Không thể kết nối đến server!\nVui lòng đảm bảo server đang chạy.");
    }

    // Connect notification signals
    connect(m_networkManager, &NetworkManager::notificationReceived,
            this, &MainWindow::onNotificationReceived);
    connect(m_networkManager, &NetworkManager::connectionLost,
            this, &MainWindow::onConnectionLost);
}

MainWindow::~MainWindow() {
}

void MainWindow::setupUi() {
    setWindowTitle("Hệ Thống Quản Lý Meeting");
    setMinimumSize(800, 600);

    // Create widgets
    m_loginWidget = new LoginWidget(m_networkManager, this);
    m_studentWidget = new StudentWidget(m_networkManager, this);
    m_teacherWidget = new TeacherWidget(m_networkManager, this);

    // Add to stacked widget
    m_stackedWidget->addWidget(m_loginWidget);    // index 0
    m_stackedWidget->addWidget(m_studentWidget);  // index 1
    m_stackedWidget->addWidget(m_teacherWidget);  // index 2

    setCentralWidget(m_stackedWidget);

    // Connect signals
    connect(m_loginWidget, &LoginWidget::loginSuccess, this, &MainWindow::onLoginSuccess);
    connect(m_studentWidget, &StudentWidget::logoutRequested, this, &MainWindow::onLogout);
    connect(m_teacherWidget, &TeacherWidget::logoutRequested, this, &MainWindow::onLogout);

    // Start with login
    m_stackedWidget->setCurrentIndex(0);
}

void MainWindow::onLoginSuccess() {
    if (m_networkManager->getRole() == "STUDENT") {
        m_studentWidget->refresh();
        m_stackedWidget->setCurrentIndex(1);
    } else {
        m_teacherWidget->refresh();
        m_stackedWidget->setCurrentIndex(2);
    }
}

void MainWindow::onLogout() {
    m_networkManager->clearLoginInfo();
    m_stackedWidget->setCurrentIndex(0);
}

void MainWindow::onNotificationReceived(const QString &type, const QString &payload) {
    if (type == "MEETING_BOOKED") {
        QString meetingId = NetworkManager::getValue(payload, "meeting_id");
        QString studentId = NetworkManager::getValue(payload, "student_id");
        QMessageBox::information(this, "Thong bao", 
            QString("Có người đặt lịch hẹn mới!\nMeeting ID: %1").arg(meetingId));
        
    } else if (type == "MEETING_CANCELLED") {
        QString meetingId = NetworkManager::getValue(payload, "meeting_id");
        QMessageBox::information(this, "Thong bao", 
            QString("Lịch hẹn đã bị hủy!\nMeeting ID: %1").arg(meetingId));
        
    } else if (type == "NEW_JOIN_REQUEST") {
        // Actionable notification - can approve/reject directly
        QString requestId = NetworkManager::getValue(payload, "request_id");
        QString groupId = NetworkManager::getValue(payload, "group_id");
        QString userName = NetworkManager::getValue(payload, "user_name");
        QString groupName = NetworkManager::getValue(payload, "group_name");
        QString note = NetworkManager::getValue(payload, "note");
        
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Yêu cầu vào nhóm");
        QString msgText = QString("Người dùng: %1\nNhóm: %2\nRequest ID: %3")
                       .arg(userName.isEmpty() ? "Unknown" : userName,
                            groupName.isEmpty() ? groupId : groupName,
                            requestId);
        if (!note.isEmpty()) {
            msgText += QString("\nGhi chú: %1").arg(note);
        }
        msgBox.setText(msgText);
        msgBox.setInformativeText("Bạn muốn xử lý yêu cầu này?");
        
        QPushButton *approveBtn = msgBox.addButton("Duyet", QMessageBox::AcceptRole);
        QPushButton *rejectBtn = msgBox.addButton("Tu choi", QMessageBox::DestructiveRole);
        QPushButton *laterBtn = msgBox.addButton("De sau", QMessageBox::RejectRole);
        
        msgBox.exec();
        
        if (msgBox.clickedButton() == approveBtn) {
            QString cmd = QString("APPROVE_JOIN_REQUEST|token=%1;request_id=%2\r\n")
                              .arg(m_networkManager->getToken(), requestId);
            QString response = m_networkManager->sendRequest(cmd);
            int status = NetworkManager::getStatusCode(response);
            if (status == 200) {
                QMessageBox::information(this, "Thành công", "Đã duyệt yêu cầu!");
            } else {
                QMessageBox::warning(this, "Lỗi", "Không thể duyệt yêu cầu!");
            }
        } else if (msgBox.clickedButton() == rejectBtn) {
            QString cmd = QString("REJECT_JOIN_REQUEST|token=%1;request_id=%2\r\n")
                              .arg(m_networkManager->getToken(), requestId);
            QString response = m_networkManager->sendRequest(cmd);
            int status = NetworkManager::getStatusCode(response);
            if (status == 200) {
                QMessageBox::information(this, "Thành công", "Đã từ chối yêu cầu!");
            } else {
                QMessageBox::warning(this, "Lỗi", "Không thể từ chối yêu cầu!");
            }
        }
        
    } else if (type == "GROUP_APPROVED") {
        QString groupId = NetworkManager::getValue(payload, "group_id");
        QMessageBox::information(this, "Thông báo", 
            QString("Yêu cầu gia nhập nhóm đã được duyệt!\nGroup ID: %1").arg(groupId));
        
    } else if (type == "GROUP_REJECTED") {
        QString groupId = NetworkManager::getValue(payload, "group_id");
        QMessageBox::information(this, "Thông báo", 
            QString("Yêu cầu gia nhập nhóm bị từ chối!\nGroup ID: %1").arg(groupId));
        
    } else {
        QMessageBox::information(this, "Thông báo", QString("Loại: %1").arg(type));
    }
}

void MainWindow::onConnectionLost() {
    QMessageBox::warning(this, "Thông báo", "Kết nối đến server đã bị mất!");
    m_stackedWidget->setCurrentIndex(0);
}

void MainWindow::showNotification(const QString &title, const QString &message) {
    QMessageBox::information(this, title, message);
}
