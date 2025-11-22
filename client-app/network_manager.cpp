#include "network_manager.h"

NetworkManager::NetworkManager(QObject *parent) : QObject(parent)
{
    m_socket = new QTcpSocket(this);

    // 绑定 QtSocket 的内置信号到我们自己的槽
    connect(m_socket, &QTcpSocket::connected, this, &NetworkManager::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &NetworkManager::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &NetworkManager::onError);
}

NetworkManager::~NetworkManager() {}

void NetworkManager::connectToServer(const QString& host, quint16 port)
{
    qInfo() << "连接到服务器..." << host << ":" << port;
    m_socket->connectToHost(host, port);
}

// 当收到服务器数据时 (JSON解析)
void NetworkManager::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    qDebug() << "收到服务器响应:" << data;

    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        qWarning() << "收到无效的JSON响应 (非JSON)";
        emit generalError("收到无效的服务器响应 (非JSON)");
        return;
    }

    QJsonObject response = jsonDoc.object();
    QString status = response["status"].toString();
    QString message = response["message"].toString();

    QString action = response["action_response"].toString();

    // 路由 "失败" 响应
    if (status == "error") {
        qWarning() << "服务器返回错误:" << message << " (Action: " << action << ")";

        // 现在我们可以根据 action 路由到特定的错误信号
        if (action == "login") {
            emit loginFailed(message); // 发出特定的 "登录失败" 信号
        }
        // else if (action == "search_flights") {
        //     emit searchFailed(message); // 后续要添加这个信号
        else {
            // 对于所有其他错误，或 action 未知的错误，发通用错误
            emit generalError(message);
        }
        return;
    }

    // 路由 "成功" 响应
    // action字段一定不能为空！
    if (action.isEmpty()) {
        return;
    }

    // 开始路由
    if (action == "login") {
        // 登录响应
        emit loginSuccess(response["data"].toObject());
    }
    else if (action == "search_flights") {
        // 航班查询响应
        emit searchResults(response["data"].toArray());
    }
    /*
    else if (action == "admin_add_flight") {
        emit adminOpSuccess(message);
    }
    */
    else {
        // 收到一个服务器认识、但客户端不认识的 action
        qWarning() << "收到未知的成功响应 action: " << action;
    }
}

void NetworkManager::onConnected() {
    qInfo() << "已连接到服务器!";
    emit connected();
}
void NetworkManager::onDisconnected() {
    qWarning() << "与服务器断开连接";
    emit disconnected();
}
void NetworkManager::onError(QAbstractSocket::SocketError socketError) {
    // 这个 onError 主要处理底层的TCP错误 (比如连不上服务器)
    qCritical() << "网络底层错误:" << m_socket->errorString();
    emit generalError(m_socket->errorString());
}

// 发送JSON的通用函
void NetworkManager::sendJsonRequest(const QJsonObject& request)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "未连接到服务器，无法发送消息";
        emit generalError("未连接到服务器");
        return;
    }

    QJsonDocument doc(request);
    m_socket->write(doc.toJson());
  D qDebug() << "发送JSON请求:" << request;
}


// 构建各种请求 (给UI调用)

void NetworkManager::sendLoginRequest(const QString& username, const QString& password)
{
    QJsonObject data;
    data["username"] = username;
    data["password"] = password;

    QJsonObject request;
    request["action"] = "login";
    request["data"] = data;

    sendJsonRequest(request);
}

void NetworkManager::sendSearchRequest(const QString& origin, const QString& dest, const QString& date)
{
    QJsonObject data;
    if (!origin.isEmpty()) data["origin"] = origin;
    if (!dest.isEmpty()) data["destination"] = dest;
    if (!date.isEmpty()) data["date"] = date;

    QJsonObject request;
    request["action"] = "search_flights";
    request["data"] = data;

    sendJsonRequest(request);
}
