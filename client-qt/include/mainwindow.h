#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QMessageBox>
#include "networkmanager.h"

class LoginWidget;
class StudentWidget;
class TeacherWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    NetworkManager* networkManager() { return m_networkManager; }

public slots:
    void onLoginSuccess();
    void onLogout();
    void onNotificationReceived(const QString &type, const QString &payload);
    void onConnectionLost();

private:
    void setupUi();
    void showNotification(const QString &title, const QString &message);
    void showServerConfigDialog();
    void createMenuBar();
    
    NetworkManager *m_networkManager;
    QStackedWidget *m_stackedWidget;
    LoginWidget *m_loginWidget;
    StudentWidget *m_studentWidget;
    TeacherWidget *m_teacherWidget;
};

#endif // MAINWINDOW_H
