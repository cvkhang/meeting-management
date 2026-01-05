#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QStackedWidget>
#include "networkmanager.h"

class LoginWidget : public QWidget {
    Q_OBJECT

public:
    explicit LoginWidget(NetworkManager *networkManager, QWidget *parent = nullptr);

signals:
    void loginSuccess();

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onSwitchToRegister();
    void onSwitchToLogin();

private:
    void setupUi();
    void setupLoginForm();
    void setupRegisterForm();

    NetworkManager *m_networkManager;
    QStackedWidget *m_formStack;

    // Login form
    QComboBox *m_loginRoleCombo;
    QLineEdit *m_loginUsernameEdit;
    QLineEdit *m_loginPasswordEdit;

    // Register form
    QComboBox *m_registerRoleCombo;
    QLineEdit *m_registerUsernameEdit;
    QLineEdit *m_registerPasswordEdit;
    QLineEdit *m_registerFullNameEdit;
};

#endif // LOGINWIDGET_H
