#include "loginwidget.h"
#include <QMessageBox>

LoginWidget::LoginWidget(NetworkManager *networkManager, QWidget *parent)
    : QWidget(parent)
    , m_networkManager(networkManager)
{
    setupUi();
}

void LoginWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);

    // Title
    QLabel *titleLabel = new QLabel("HỆ THỐNG QUẢN LÝ MEETING");
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #2196F3; margin-bottom: 20px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // Stacked widget for login/register
    m_formStack = new QStackedWidget();
    m_formStack->setMaximumWidth(400);

    setupLoginForm();
    setupRegisterForm();

    mainLayout->addWidget(m_formStack, 0, Qt::AlignCenter);
}

void LoginWidget::setupLoginForm() {
    QWidget *loginWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(loginWidget);

    QGroupBox *groupBox = new QGroupBox("Đăng nhập");
    groupBox->setStyleSheet("QGroupBox { font-weight: bold; }");
    QFormLayout *formLayout = new QFormLayout(groupBox);

    // Role will be auto-detected from server response

    m_loginUsernameEdit = new QLineEdit();
    m_loginUsernameEdit->setPlaceholderText("Nhập tên đăng nhập");
    formLayout->addRow("Tên đăng nhập:", m_loginUsernameEdit);

    m_loginPasswordEdit = new QLineEdit();
    m_loginPasswordEdit->setEchoMode(QLineEdit::Password);
    m_loginPasswordEdit->setPlaceholderText("Nhập mật khẩu");
    formLayout->addRow("Mật khẩu:", m_loginPasswordEdit);

    layout->addWidget(groupBox);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    QPushButton *loginBtn = new QPushButton("Đăng nhập");
    loginBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 10px; font-weight: bold; }");
    connect(loginBtn, &QPushButton::clicked, this, &LoginWidget::onLoginClicked);
    buttonLayout->addWidget(loginBtn);

    QPushButton *toRegisterBtn = new QPushButton("Đăng ký");
    toRegisterBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; padding: 10px; }");
    connect(toRegisterBtn, &QPushButton::clicked, this, &LoginWidget::onSwitchToRegister);
    buttonLayout->addWidget(toRegisterBtn);

    layout->addLayout(buttonLayout);

    m_formStack->addWidget(loginWidget);
}

void LoginWidget::setupRegisterForm() {
    QWidget *registerWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(registerWidget);

    QGroupBox *groupBox = new QGroupBox("Đăng ký tài khoản");
    groupBox->setStyleSheet("QGroupBox { font-weight: bold; }");
    QFormLayout *formLayout = new QFormLayout(groupBox);

    m_registerRoleCombo = new QComboBox();
    m_registerRoleCombo->addItem("Sinh viên", "STUDENT");
    m_registerRoleCombo->addItem("Giảng viên", "TEACHER");
    formLayout->addRow("Vai trò:", m_registerRoleCombo);

    m_registerUsernameEdit = new QLineEdit();
    m_registerUsernameEdit->setPlaceholderText("Nhập tên đăng nhập");
    formLayout->addRow("Tên đăng nhập:", m_registerUsernameEdit);

    m_registerPasswordEdit = new QLineEdit();
    m_registerPasswordEdit->setEchoMode(QLineEdit::Password);
    m_registerPasswordEdit->setPlaceholderText("Nhập mật khẩu");
    formLayout->addRow("Mật khẩu:", m_registerPasswordEdit);

    m_registerFullNameEdit = new QLineEdit();
    m_registerFullNameEdit->setPlaceholderText("Nhập họ và tên");
    formLayout->addRow("Họ và tên:", m_registerFullNameEdit);

    layout->addWidget(groupBox);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    QPushButton *registerBtn = new QPushButton("Đăng ký");
    registerBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 10px; font-weight: bold; }");
    connect(registerBtn, &QPushButton::clicked, this, &LoginWidget::onRegisterClicked);
    buttonLayout->addWidget(registerBtn);

    QPushButton *toLoginBtn = new QPushButton("Quay lại");
    toLoginBtn->setStyleSheet("QPushButton { background-color: #9E9E9E; color: white; padding: 10px; }");
    connect(toLoginBtn, &QPushButton::clicked, this, &LoginWidget::onSwitchToLogin);
    buttonLayout->addWidget(toLoginBtn);

    layout->addLayout(buttonLayout);

    m_formStack->addWidget(registerWidget);
}

void LoginWidget::onLoginClicked() {
    QString username = m_loginUsernameEdit->text().trimmed();
    QString password = m_loginPasswordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Lỗi", "Vui lòng nhập đầy đủ thông tin!");
        return;
    }

    QString command = QString("LOGIN|username=%1;password=%2\r\n")
                          .arg(username, password);
    
    QString response = m_networkManager->sendRequest(command);
    int status = NetworkManager::getStatusCode(response);

    if (status == 200) {
        QString userId = NetworkManager::getValue(response, "user_id");
        QString token = NetworkManager::getValue(response, "token");
        QString respRole = NetworkManager::getValue(response, "role");
        
        m_networkManager->setLoginInfo(userId.toInt(), token, respRole, username);
        
        m_loginPasswordEdit->clear();
        QMessageBox::information(this, "Thành công", "Đăng nhập thành công!");
        emit loginSuccess();
    } else {
        QMessageBox::warning(this, "Lỗi", "Sai tên đăng nhập hoặc mật khẩu!");
    }
}

void LoginWidget::onRegisterClicked() {
    QString role = m_registerRoleCombo->currentData().toString();
    QString username = m_registerUsernameEdit->text().trimmed();
    QString password = m_registerPasswordEdit->text();
    QString fullName = m_registerFullNameEdit->text().trimmed();

    if (username.isEmpty() || password.isEmpty() || fullName.isEmpty()) {
        QMessageBox::warning(this, "Lỗi", "Vui lòng nhập đầy đủ thông tin!");
        return;
    }

    QString command = QString("REGISTER|role=%1;username=%2;password=%3;full_name=%4\r\n")
                          .arg(role, username, password, fullName);
    
    QString response = m_networkManager->sendRequest(command);
    int status = NetworkManager::getStatusCode(response);

    if (status == 201) {
        QMessageBox::information(this, "Thành công", "Đăng ký thành công! Vui lòng đăng nhập.");
        m_registerUsernameEdit->clear();
        m_registerPasswordEdit->clear();
        m_registerFullNameEdit->clear();
        onSwitchToLogin();
    } else if (status == 409) {
        QMessageBox::warning(this, "Lỗi", "Tên đăng nhập đã tồn tại!");
    } else {
        QMessageBox::warning(this, "Lỗi", "Đăng ký thất bại!");
    }
}

void LoginWidget::onSwitchToRegister() {
    m_formStack->setCurrentIndex(1);
}

void LoginWidget::onSwitchToLogin() {
    m_formStack->setCurrentIndex(0);
}
