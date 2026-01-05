#include "serverconfigdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QTcpSocket>

ServerConfigDialog::ServerConfigDialog(QWidget *parent)
    : QDialog(parent)
    , m_serverIp("127.0.0.1")
    , m_serverPort(8080)
{
    setWindowTitle("Cấu hình kết nối Server");
    setModal(true);
    setMinimumWidth(400);
    
    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Title
    QLabel *titleLabel = new QLabel("Nhập địa chỉ Server để kết nối:");
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    mainLayout->addWidget(titleLabel);
    
    // Form layout
    QFormLayout *formLayout = new QFormLayout();
    
    // IP input
    m_ipEdit = new QLineEdit();
    m_ipEdit->setPlaceholderText("Ví dụ: 192.168.1.100 hoặc 127.0.0.1");
    formLayout->addRow("Địa chỉ IP:", m_ipEdit);
    
    // Port input
    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(8080);
    formLayout->addRow("Port:", m_portSpin);
    
    mainLayout->addLayout(formLayout);
    mainLayout->addSpacing(10);
    
    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    
    m_testBtn = new QPushButton("Kiểm tra kết nối");
    m_testBtn->setStyleSheet("background-color: #2196F3; color: white;");
    connect(m_testBtn, &QPushButton::clicked, this, &ServerConfigDialog::onTestConnection);
    
    m_connectBtn = new QPushButton("Kết nối");
    m_connectBtn->setStyleSheet("background-color: #4CAF50; color: white;");
    connect(m_connectBtn, &QPushButton::clicked, this, &ServerConfigDialog::onAccept);
    
    m_cancelBtn = new QPushButton("Hủy");
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    
    btnLayout->addWidget(m_testBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_connectBtn);
    btnLayout->addWidget(m_cancelBtn);
    
    mainLayout->addLayout(btnLayout);
    
    // Load saved settings
    loadSettings();
}

QString ServerConfigDialog::getServerIp() const {
    return m_serverIp;
}

int ServerConfigDialog::getServerPort() const {
    return m_serverPort;
}

void ServerConfigDialog::onTestConnection() {
    QString ip = m_ipEdit->text().trimmed();
    int port = m_portSpin->value();
    
    if (ip.isEmpty()) {
        QMessageBox::warning(this, "Lỗi", "Vui lòng nhập địa chỉ IP!");
        return;
    }
    
    // Test connection
    QTcpSocket testSocket;
    testSocket.connectToHost(ip, port);
    
    if (testSocket.waitForConnected(3000)) {
        QMessageBox::information(this, "Thành công", 
            QString("Kết nối tới %1:%2 thành công!").arg(ip).arg(port));
        testSocket.disconnectFromHost();
    } else {
        QMessageBox::warning(this, "Thất bại", 
            QString("Không thể kết nối tới %1:%2\n\nLỗi: %3")
            .arg(ip).arg(port).arg(testSocket.errorString()));
    }
}

void ServerConfigDialog::onAccept() {
    QString ip = m_ipEdit->text().trimmed();
    int port = m_portSpin->value();
    
    if (ip.isEmpty()) {
        QMessageBox::warning(this, "Lỗi", "Vui lòng nhập địa chỉ IP!");
        return;
    }
    
    m_serverIp = ip;
    m_serverPort = port;
    
    saveSettings();
    accept();
}

void ServerConfigDialog::loadSettings() {
    QSettings settings("MeetingApp", "ClientConfig");
    m_serverIp = settings.value("server/ip", "127.0.0.1").toString();
    m_serverPort = settings.value("server/port", 8080).toInt();
    
    m_ipEdit->setText(m_serverIp);
    m_portSpin->setValue(m_serverPort);
}

void ServerConfigDialog::saveSettings() {
    QSettings settings("MeetingApp", "ClientConfig");
    settings.setValue("server/ip", m_serverIp);
    settings.setValue("server/port", m_serverPort);
}
