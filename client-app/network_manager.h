/*
这里的实现和server-app中的单例tcp_server.h高度相似，
用于在客户端实现统一的信息收发，和服务器端进行交流。
*/
#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    static NetworkManager& instance() {
        static NetworkManager instance;
        return instance;
    }

    void connectToServer(const QString& host, quint16 port);

    // 定义所有"发送"函数
    // 这是给UI界面调用的 (例如: on_loginButton_clicked)
    void sendLoginRequest(const QString& username, const QString& password);
    void sendSearchRequest(const QString& origin, const QString& dest, const QString& date);
    // ... (注意，每个action都对应一个发送函数，如果后续要新增这里也要加)

signals:
    // 定义所有"接收"信号
    // 这是发给UI界面的 (例如: onLoginSuccess)
    void connected();
    void disconnected();
    void loginSuccess(const QJsonObject& userData);
    void loginFailed(const QString& message);
    void searchResults(const QJsonArray& flights);
    // ... (如果后续要加加在这里)
    void generalError(const QString& message);


private slots:
    void onConnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);
    void onDisconnected();

private:
    explicit NetworkManager(QObject *parent = nullptr); // 和server-app中类似私有构造，防止出现多个实例
    ~NetworkManager();
    // 禁用拷贝
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    QTcpSocket *m_socket;

    void sendJsonRequest(const QJsonObject& request);
};

#endif // NETWORKMANAGER_H
