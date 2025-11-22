/*
用来监视tcp请求
*/

#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "database_manager.h"


class TcpServer : public QObject
{
    Q_OBJECT
    // 这一行是允许这个类使用Qt的信息与槽机制

public:
    explicit TcpServer(QObject *parent = nullptr);
    void startServer(quint16 port);

private slots:
    void onNewConnection();
    // 当有新客户端连接时，就用这个函数
    void onReadyRead();
    // 客户端发来数据，调用这个
    void onDisconnected();
    // 客户端断开连接，调用这个

private:
    QTcpServer *m_server;
    QHash<QString, QTcpSocket*> clients; // key = 客户端 tag, value = socket

    // 这是功能分发函数，看其中的action内容来决定调用哪个具体函数
    QJsonObject handleRequest(const QJsonObject& request);

    QJsonObject handleLogin(const QJsonObject& data);
    QJsonObject handleSearchFlights(const QJsonObject& data);
    // 注意，每一个action或者说每一个具体功能都需要一个handle函数！！！！

    // 辅助函数，将JSON响应发回客户端
    void sendJsonResponse(QTcpSocket* socket, const QJsonObject& response);
};

#endif // TCPSERVER_H
