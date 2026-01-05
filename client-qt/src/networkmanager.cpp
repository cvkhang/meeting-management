#include "networkmanager.h"
#include <cstring>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , m_socketFd(-1)
    , m_connected(false)
    , m_loggedIn(false)
    , m_userId(0)
    , m_listenerThread(nullptr)
    , m_listener(nullptr)
{
}

NetworkManager::~NetworkManager() {
    disconnectFromServer();
}

bool NetworkManager::connectToServer(const QString &ip, int port) {
    if (m_connected) return true;

    // Create socket
    m_socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socketFd < 0) {
      return false;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.toStdString().c_str(), &serverAddr.sin_addr) <= 0) {
      ::close(m_socketFd);
      m_socketFd = -1;
      return false;
    }

    if (::connect(m_socketFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
      ::close(m_socketFd);
      m_socketFd = -1;
      return false;
    }

    m_connected = true;
    return true;
}

void NetworkManager::disconnectFromServer() {
    stopNotificationListener();
    
    if (m_socketFd >= 0) {
      ::close(m_socketFd);
      m_socketFd = -1;
    }
    m_connected = false;
    m_loggedIn = false;
}

bool NetworkManager::isConnected() const {
    return m_connected;
}

QString NetworkManager::sendRequest(const QString &command) {
    if (!m_connected) return QString();

    QMutexLocker locker(&m_socketMutex);

    // Send using socket
    std::string cmdStr = command.toStdString();
    if (send(m_socketFd, cmdStr.c_str(), cmdStr.length(), 0) < 0) {
      return QString();
    }

    // Receive response
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    
    int n = recv(m_socketFd, buffer, BUFFER_SIZE - 1, 0);
    if (n <= 0) {
      return QString();
    }
    buffer[n] = '\0';

    return QString::fromUtf8(buffer);
}

void NetworkManager::setLoginInfo(int userId, const QString &token, const QString &role, const QString &fullName) {
    m_userId = userId;
    m_token = token;
    m_role = role;
    m_fullName = fullName;
    m_loggedIn = true;
    
    startNotificationListener();
}

void NetworkManager::clearLoginInfo() {
    stopNotificationListener();
    m_userId = 0;
    m_token.clear();
    m_role.clear();
    m_fullName.clear();
    m_loggedIn = false;
}

int NetworkManager::getStatusCode(const QString &response) {
    int code = 0;
    QStringList parts = response.split('|');
    if (!parts.isEmpty()) {
      code = parts[0].toInt();
    }
    return code;
}

QString NetworkManager::getValue(const QString &response, const QString &key) {
    int pipeIndex = response.indexOf('|');
    if (pipeIndex < 0) return QString();

    QString payload = response.mid(pipeIndex + 1).trimmed();
    payload.remove('\r');
    payload.remove('\n');

    QStringList pairs = payload.split(';');
    for (const QString &pair : pairs) {
      int eqIndex = pair.indexOf('=');
      if (eqIndex > 0) {
        QString k = pair.left(eqIndex);
        QString v = pair.mid(eqIndex + 1);
        if (k == key) {
          return v;
        }
      }
    }
    return QString();
}

void NetworkManager::startNotificationListener() {
    if (m_listenerThread) return;

    m_listenerThread = new QThread();
    m_listener = new NotificationListener(m_socketFd, &m_socketMutex);
    m_listener->moveToThread(m_listenerThread);

    connect(m_listenerThread, &QThread::started, m_listener, &NotificationListener::process);
    connect(m_listener, &NotificationListener::finished, m_listenerThread, &QThread::quit);
    connect(m_listener, &NotificationListener::finished, m_listener, &QObject::deleteLater);
    connect(m_listenerThread, &QThread::finished, m_listenerThread, &QObject::deleteLater);
    
    connect(m_listener, &NotificationListener::notificationReceived,
            this, &NetworkManager::notificationReceived);
    connect(m_listener, &NotificationListener::connectionLost,
            this, &NetworkManager::connectionLost);

    m_listenerThread->start();
}

void NetworkManager::stopNotificationListener() {
    if (m_listener) {
      m_listener->stop();
    }
    if (m_listenerThread) {
      m_listenerThread->quit();
      m_listenerThread->wait(1000);
      m_listenerThread = nullptr;
      m_listener = nullptr;
    }
}

// NotificationListener implementation
NotificationListener::NotificationListener(int socketFd, QMutex *mutex)
    : m_socketFd(socketFd)
    , m_socketMutex(mutex)
    , m_running(true)
{
}

void NotificationListener::stop() {
    m_running = false;
}

void NotificationListener::process() {
    char buffer[BUFFER_SIZE];
    
    while (m_running) {
      fd_set readFds;
      struct timeval tv;
        
      FD_ZERO(&readFds);
      FD_SET(m_socketFd, &readFds);
      
      tv.tv_sec = 0;
      tv.tv_usec = 100000; // 100ms timeout
      
      int result = select(m_socketFd + 1, &readFds, nullptr, nullptr, &tv);
      
      if (result > 0 && FD_ISSET(m_socketFd, &readFds)) {
        // Don't lock mutex here - we only read notifications
        // The main thread locks when sending requests
        memset(buffer, 0, BUFFER_SIZE);
        int n = recv(m_socketFd, buffer, BUFFER_SIZE - 1, MSG_PEEK);
        
        if (n > 0) {
          QString data = QString::fromUtf8(buffer);
          if (data.startsWith("NTF|")) {
            // Actually consume the data
            recv(m_socketFd, buffer, n, 0);
            
            QString type = NetworkManager::getValue(data, "type");
            emit notificationReceived(type, data);
          }
        } else if (n == 0) {
          emit connectionLost();
          break;
        }
      }
    }
    
    emit finished();
}
