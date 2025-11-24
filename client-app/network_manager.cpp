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

        if (action == "login") {
            emit loginFailed(message);
        }
        else if (action == "search_flights") {
            emit generalError(message);
        }
        else if (action == "register") {
            emit registerFailed(message);
        }
        else if (action == "book_flight") {
            emit bookingFailed(message);
        }
        else if (action == "get_my_orders") {
            emit generalError(message);
        }
        else if (action == "cancel_order") {
            emit cancelOrderFailed(message);
        }
        else {
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
        emit loginSuccess(response["data"].toObject());
    }
    else if (action == "search_flights") {
        emit searchResults(response["data"].toArray());
    }
    else if (action == "register") {
        emit registerSuccess(message);
    }
    else if (action == "book_flight") {
        emit bookingSuccess(response["data"].toObject());
    }
    else if (action == "get_my_orders") {
        emit myOrdersResult(response["data"].toArray());
    }
    else if (action == "cancel_order") {
        emit cancelOrderSuccess(message);
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
    qDebug() << "发送JSON请求:" << request;
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

void NetworkManager::sendRegisterRequest(const QString& username, const QString& password)
{
    QJsonObject data;
    data["username"] = username;
    data["password"] = password;

    QJsonObject request;
    request["action"] = "register";
    request["data"] = data;

    sendJsonRequest(request);
}

void NetworkManager::bookFlightRequest(int userId, int flightId)
{
    QJsonObject data;
    data["user_id"] = userId;
    data["flight_id"] = flightId;

    QJsonObject request;
    request["action"] = "book_flight";
    request["data"] = data;

    sendJsonRequest(request);
}

void NetworkManager::getMyOrdersRequest(int userId)
{
    QJsonObject data;
    data["user_id"] = userId;

    QJsonObject request;
    request["action"] = "get_my_orders";
    request["data"] = data;

    sendJsonRequest(request);
}

void NetworkManager::cancelOrderRequest(int bookingId)
{
    QJsonObject data;
    data["booking_id"] = bookingId;

    QJsonObject request;
    request["action"] = "cancel_order";
    request["data"] = data;

    sendJsonRequest(request);
}
