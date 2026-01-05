#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QString>
#include <QThread>
#include <QMutex>

// socket headers
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 4096

class NotificationListener;

class NetworkManager : public QObject {
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();

    bool connectToServer(const QString &ip, int port);
    void disconnectFromServer();
    bool isConnected() const;

    QString sendRequest(const QString &command);
    
    // Getters
    int getUserId() const { return m_userId; }
    QString getToken() const { return m_token; }
    QString getRole() const { return m_role; }
    QString getFullName() const { return m_fullName; }
    bool isLoggedIn() const { return m_loggedIn; }

    // Setters after login
    void setLoginInfo(int userId, const QString &token, const QString &role, const QString &fullName);
    void clearLoginInfo();

    // Parse helpers
    static int getStatusCode(const QString &response);
    static QString getValue(const QString &response, const QString &key);

signals:
    void notificationReceived(const QString &type, const QString &payload);
    void connectionLost();

private:
    int m_socketFd;
    bool m_connected;
    bool m_loggedIn;
    int m_userId;
    QString m_token;
    QString m_role;
    QString m_fullName;
    
    QThread *m_listenerThread;
    NotificationListener *m_listener;
    QMutex m_socketMutex;

    void startNotificationListener();
    void stopNotificationListener();
};

// Background thread for listening to notifications
class NotificationListener : public QObject {
    Q_OBJECT

public:
    NotificationListener(int socketFd, QMutex *mutex);
    void stop();

public slots:
    void process();

signals:
    void notificationReceived(const QString &type, const QString &payload);
    void connectionLost();
    void finished();

private:
    int m_socketFd;
    QMutex *m_socketMutex;
    bool m_running;
};

#endif // NETWORKMANAGER_H
