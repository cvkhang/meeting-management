#ifndef SERVERCONFIGDIALOG_H
#define SERVERCONFIGDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QSettings>

class ServerConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit ServerConfigDialog(QWidget *parent = nullptr);
    
    QString getServerIp() const;
    int getServerPort() const;
    
private slots:
    void onTestConnection();
    void onAccept();
    
private:
    void loadSettings();
    void saveSettings();
    
    QLineEdit *m_ipEdit;
    QSpinBox *m_portSpin;
    QPushButton *m_testBtn;
    QPushButton *m_connectBtn;
    QPushButton *m_cancelBtn;
    
    QString m_serverIp;
    int m_serverPort;
};

#endif // SERVERCONFIGDIALOG_H
