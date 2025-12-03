#include "network_manager.h"

#include <QTimer>
#include <QDateTime>
#include <QRandomGenerator>

NetworkManager::NetworkManager(QObject *parent) : QObject(parent), m_tagRegistered(false)
{
#ifdef USE_FAKE_SERVER
    m_socket = nullptr;
#else
    m_socket = new QTcpSocket(this);

    // 绑定 QtSocket 的内置信号到我们自己的槽
    connect(m_socket, &QTcpSocket::connected, this, &NetworkManager::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &NetworkManager::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &NetworkManager::onError);
#endif
}

NetworkManager::~NetworkManager() {}

void NetworkManager::connectToServer(const QString& host, quint16 port)
{
#ifdef USE_FAKE_SERVER
    Q_UNUSED(host);
    Q_UNUSED(port);
    qInfo() << "[FAKE SERVER] 跳过真实服务器连接";
    QTimer::singleShot(0, this, [this]() {
        emit connected();
    });
#else
    qInfo() << "连接到服务器..." << host << ":" << port;
    m_socket->connectToHost(host, port);
#endif
}

// 当收到服务器数据时 (JSON解析)
#ifndef USE_FAKE_SERVER
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

    // 检查是否是tag注册响应（没有action_response字段）
    if (response.contains("tag") || (status == "success" && message == "Tag registered")) {
        if (status == "success") {
            m_tagRegistered = true;
            qInfo() << "Tag注册成功";
            emit tagRegistered();
        } else {
            qWarning() << "Tag注册失败:" << message;
            emit tagRegistrationFailed(message);
        }
        return;
    }

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
        else if (action == "update_profile") {
            emit profileUpdateFailed(message);
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
    else if (action == "update_profile") {
        emit profileUpdateSuccess(message, response["data"].toObject());
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
#else
void NetworkManager::onReadyRead() {}
#endif

void NetworkManager::onConnected() {
    qInfo() << "已连接到服务器!";
    emit connected();
    
#ifndef USE_FAKE_SERVER
    // 连接成功后自动发送tag注册（仅在真实服务器模式下）
    QString tag = generateUniqueTag();
    m_clientTag = tag;
    sendTagRegistration(tag);
#else
    // 假服务器模式下直接标记tag已注册
    m_tagRegistered = true;
    emit tagRegistered();
#endif
}
void NetworkManager::onDisconnected() {
    qWarning() << "与服务器断开连接";
    m_tagRegistered = false; // 重置tag注册状态
    emit disconnected();
}
void NetworkManager::onError(QAbstractSocket::SocketError socketError) {
    // 这个 onError 主要处理底层的TCP错误 (比如连不上服务器)
    qCritical() << "网络底层错误:" << m_socket->errorString();
    emit generalError(m_socket->errorString());
}

// Tag注册功能实现
void NetworkManager::sendTagRegistration(const QString& tag)
{
#ifdef USE_FAKE_SERVER
    Q_UNUSED(tag);
    // 模拟tag注册成功
    QTimer::singleShot(100, this, [this]() {
        m_tagRegistered = true;
        emit tagRegistered();
    });
    return;
#else
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "未连接到服务器，无法注册tag";
        emit tagRegistrationFailed("未连接到服务器");
        return;
    }

    QJsonObject request;
    request["tag"] = tag;

    QJsonDocument doc(request);
    m_socket->write(doc.toJson());
    qDebug() << "发送tag注册请求:" << tag;
#endif
}

bool NetworkManager::isTagRegistered() const
{
    return m_tagRegistered;
}

QString NetworkManager::generateUniqueTag() const
{
    // 生成基于时间戳和随机数的唯一tag
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    int random = QRandomGenerator::global()->bounded(1000, 9999);
    return QString("client_%1_%2").arg(timestamp).arg(random);
}

// 发送JSON的通用函
void NetworkManager::sendJsonRequest(const QJsonObject& request)
{
#ifdef USE_FAKE_SERVER
    qWarning() << "[FAKE SERVER] sendJsonRequest 被调用，但当前为本地模拟模式";
    return;
#else
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "未连接到服务器，无法发送消息";
        emit generalError("未连接到服务器");
        return;
    }

    if (!m_tagRegistered) {
        qWarning() << "Tag未注册，无法发送业务请求";
        emit generalError("Tag未注册，请先完成tag注册");
        return;
    }

    QJsonDocument doc(request);
    m_socket->write(doc.toJson());
    qDebug() << "发送JSON请求:" << request;
#endif
}


// 构建各种请求 (给UI调用)

void NetworkManager::sendLoginRequest(const QString& username, const QString& password)
{
#ifdef USE_FAKE_SERVER
    Q_UNUSED(password);
    emitFakeLoginResponse(username);
    return;
#endif
    QJsonObject data;
    data["username"] = username;
    data["password"] = password;

    QJsonObject request;
    request["action"] = "login";
    request["data"] = data;

    sendJsonRequest(request);
}

void NetworkManager::updateProfileRequest(int userId, const QString &username, const QString &password)
{
#ifdef USE_FAKE_SERVER
    emitFakeProfileUpdateResponse(userId, username);
    Q_UNUSED(password);
    return;
#endif
    QJsonObject data;
    data["user_id"] = userId;
    data["username"] = username;
    data["password"] = password;

    QJsonObject request;
    request["action"] = "update_profile";
    request["data"] = data;

    sendJsonRequest(request);
}

void NetworkManager::sendSearchRequest(const QString& origin, const QString& dest, const QString& date,
                                       const QString& cabinClass,
                                       const QStringList& passengerTypes)
{
#ifdef USE_FAKE_SERVER
    emitFakeSearchResults(origin, dest, date);
    return;
#endif
    QJsonObject data;
    if (!origin.isEmpty()) data["origin"] = origin;
    if (!dest.isEmpty()) data["destination"] = dest;
    if (!date.isEmpty()) data["date"] = date;
    if (!cabinClass.isEmpty()) data["cabin_class"] = cabinClass;
    if (!passengerTypes.isEmpty()) {
        data["passenger_types"] = QJsonArray::fromStringList(passengerTypes);
    }

    QJsonObject request;
    request["action"] = "search_flights";
    request["data"] = data;

    sendJsonRequest(request);
}

void NetworkManager::sendRegisterRequest(const QString& username, const QString& password)
{
#ifdef USE_FAKE_SERVER
    Q_UNUSED(password);
    emitFakeRegisterResponse(username);
    return;
#endif
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
#ifdef USE_FAKE_SERVER
    emitFakeBookingResponse(userId, flightId);
    return;
#endif
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
#ifdef USE_FAKE_SERVER
    emitFakeOrdersResponse(userId);
    return;
#endif
    QJsonObject data;
    data["user_id"] = userId;

    QJsonObject request;
    request["action"] = "get_my_orders";
    request["data"] = data;

    sendJsonRequest(request);
}

void NetworkManager::cancelOrderRequest(int bookingId)
{
#ifdef USE_FAKE_SERVER
    emitFakeCancelResponse(bookingId);
    return;
#endif
    QJsonObject data;
    data["booking_id"] = bookingId;

    QJsonObject request;
    request["action"] = "cancel_order";
    request["data"] = data;

    sendJsonRequest(request);
}

#ifdef USE_FAKE_SERVER
void NetworkManager::emitFakeLoginResponse(const QString& username)
{
    QJsonObject user;
    user["user_id"] = 1;
    user["username"] = username.isEmpty() ? QStringLiteral("demo_user") : username;
    user["is_admin"] = 0;

    QTimer::singleShot(100, this, [this, user]() {
        emit loginSuccess(user);
    });
}

void NetworkManager::emitFakeSearchResults(const QString& origin, const QString& dest, const QString& date)
{
    QJsonArray flights;

    QJsonObject flight1;
    flight1["flight_id"] = 101;
    flight1["flight_number"] = QStringLiteral("CA101");
    flight1["origin"] = origin.isEmpty() ? QStringLiteral("北京") : origin;
    flight1["destination"] = dest.isEmpty() ? QStringLiteral("上海") : dest;
    flight1["departure_time"] = date.isEmpty() ? QStringLiteral("2025-12-01T08:00:00") : date + "T08:00:00";
    flight1["arrival_time"] = QStringLiteral("2025-12-01T10:15:00");
    flight1["price"] = 850;
    flight1["remaining_seats"] = 23;

    QJsonObject flight2 = flight1;
    flight2["flight_id"] = 102;
    flight2["flight_number"] = QStringLiteral("MU233");
    flight2["departure_time"] = date.isEmpty() ? QStringLiteral("2025-12-01T14:30:00") : date + "T14:30:00";
    flight2["arrival_time"] = QStringLiteral("2025-12-01T17:05:00");
    flight2["price"] = 920;
    flight2["remaining_seats"] = 12;

    flights.append(flight1);
    flights.append(flight2);

    QTimer::singleShot(150, this, [this, flights]() {
        emit searchResults(flights);
    });
}

void NetworkManager::emitFakeRegisterResponse(const QString& username)
{
    QString message = QStringLiteral("注册成功 (本地模拟) : %1")
                          .arg(username.isEmpty() ? QStringLiteral("demo_user") : username);
    QTimer::singleShot(120, this, [this, message]() {
        emit registerSuccess(message);
    });
}

void NetworkManager::emitFakeBookingResponse(int userId, int flightId)
{
    QJsonObject booking;
    booking["booking_id"] = 500 + flightId;
    booking["user_id"] = userId;
    booking["flight_id"] = flightId;
    booking["status"] = QStringLiteral("confirmed");

    QTimer::singleShot(150, this, [this, booking]() {
        emit bookingSuccess(booking);
    });
}

void NetworkManager::emitFakeOrdersResponse(int userId)
{
    QJsonArray orders;

    QJsonObject order1;
    order1["booking_id"] = 700;
    order1["flight_id"] = 101;
    order1["status"] = QStringLiteral("confirmed");
    order1["flight_number"] = QStringLiteral("CA101");
    order1["origin"] = QStringLiteral("北京");
    order1["destination"] = QStringLiteral("上海");
    order1["departure_time"] = QStringLiteral("2025-12-01T08:00:00");
    order1["user_id"] = userId;

    QJsonObject order2 = order1;
    order2["booking_id"] = 701;
    order2["flight_id"] = 105;
    order2["destination"] = QStringLiteral("深圳");
    order2["departure_time"] = QStringLiteral("2025-12-05T19:20:00");

    orders.append(order1);
    orders.append(order2);

    QTimer::singleShot(150, this, [this, orders]() {
        emit myOrdersResult(orders);
    });
}

void NetworkManager::emitFakeCancelResponse(int bookingId)
{
    QString message = QStringLiteral("订单 %1 已取消 (本地模拟)").arg(bookingId);
    QTimer::singleShot(100, this, [this, message]() {
        emit cancelOrderSuccess(message);
    });
}

void NetworkManager::emitFakeProfileUpdateResponse(int userId, const QString &username)
{
    QJsonObject user;
    user["user_id"] = userId <= 0 ? 1 : userId;
    user["username"] = username.isEmpty() ? QStringLiteral("demo_user") : username;
    user["is_admin"] = 0;

    QString message = QStringLiteral("个人信息已更新 (本地模拟)");
    QTimer::singleShot(120, this, [this, message, user]() {
        emit profileUpdateSuccess(message, user);
    });
}
#endif // USE_FAKE_SERVER
