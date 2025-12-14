#include "network_manager.h"

#include "app_session.h"

#include <QTimer>
#include <QDateTime>
#include <QRandomGenerator>
#include <QtEndian>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent), m_socket(nullptr), m_tagRegistered(false), m_reconnectPending(false), m_lastPort(0)
{
    m_socket = new QTcpSocket(this);

    // 绑定 QtSocket 的内置信号到我们自己的槽
    connect(m_socket, &QTcpSocket::connected, this, &NetworkManager::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &NetworkManager::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &NetworkManager::onError);
}

NetworkManager::~NetworkManager() {}

void NetworkManager::connectToServer(const QString &host, quint16 port)
{
    m_lastHost = host;
    m_lastPort = port;
    qInfo() << "连接到服务器" << host << ":" << port;
    m_socket->connectToHost(host, port);
}

// 当收到服务器数据时 (JSON解析)
void NetworkManager::onReadyRead()
{
    m_receiveBuffer.append(m_socket->readAll());
    processLengthPrefixedBuffer();
}

void NetworkManager::processLengthPrefixedBuffer()
{
    const int headerSize = static_cast<int>(sizeof(quint32));
    const quint32 maxFrameSize = 4u * 1024u * 1024u;

    while (true)
    {
        if (m_receiveBuffer.size() < headerSize)
        {
            return;
        }

        quint32 frameLength = qFromBigEndian<quint32>(
            reinterpret_cast<const uchar *>(m_receiveBuffer.constData()));

        if (frameLength == 0 || frameLength > maxFrameSize)
        {
            qWarning() << "收到异常的帧长度:" << frameLength;
            emit generalError("收到异常的服务器响应 (帧长度)");
            m_receiveBuffer.clear();
            return;
        }

        const int totalNeeded = headerSize + static_cast<int>(frameLength);
        if (m_receiveBuffer.size() < totalNeeded)
        {
            return;
        }

        QByteArray payload = m_receiveBuffer.mid(headerSize, frameLength);
        m_receiveBuffer.remove(0, totalNeeded);

        qDebug() << "收到服务器响应:" << payload;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(payload);
        if (jsonDoc.isNull() || !jsonDoc.isObject())
        {
            qWarning() << "收到无效的JSON响应 (非JSON)";
            emit generalError("收到无效的服务器响应 (非JSON)");
            continue;
        }

        handleResponseObject(jsonDoc.object());
    }
}

void NetworkManager::handleResponseObject(const QJsonObject &response)
{
    QString status = response.value("status").toString();
    QString message = response.value("message").toString();

    if (response.contains("tag") || (status == "success" && message == "Tag registered"))
    {
        if (status == "success")
        {
            m_tagRegistered = true;
            qInfo() << "Tag注册成功";
            emit tagRegistered();
        }
        else
        {
            qWarning() << "Tag注册失败:" << message;
            emit tagRegistrationFailed(message);
        }
        return;
    }

    QString action = response.value("action_response").toString();
    if (action.isEmpty() && response.contains("action"))
    {
        action = response.value("action").toString();
    }

    QJsonObject requestPayload = detachPendingPayload(action);

    if (action.isEmpty())
    {
        qWarning() << "无法确定响应对应的 action，丢弃该响应";
        if (status == "error" && !message.isEmpty())
        {
            const QString suppressed = QStringLiteral("Invalid JSON format");
            if (message.compare(suppressed, Qt::CaseInsensitive) != 0)
            {
                emit generalError(message);
            }
        }
        return;
    }

    if (status == "error")
    {
        qWarning() << "服务器返回错误:" << message << " (Action: " << action << ")";
        emitActionFailed(action, message);
        return;
    }

    if (action == "login")
    {
        emit loginSuccess(response.value("data").toObject());
    }
    else if (action == "search_flights")
    {
        emit searchResults(response.value("data").toArray());
    }
    else if (action == "register")
    {
        emit registerSuccess(message);
    }
    else if (action == "book_flight")
    {
        emit bookingSuccess(response.value("data").toObject());
    }
    else if (action == "get_my_orders")
    {
        emit myOrdersResult(response.value("data").toArray());
    }
    else if (action == "cancel_order")
    {
        emit cancelOrderSuccess(message);
    }
    else if (action == "update_profile")
    {
        QJsonObject userData = response.value("data").toObject();
        if (userData.isEmpty())
        {
            userData = requestPayload;
            userData.remove("password");
            if (!userData.contains("user_id") || userData.value("user_id").toInt() <= 0)
            {
                userData["user_id"] = AppSession::instance().userId();
            }
            if (!userData.contains("username") || userData.value("username").toString().isEmpty())
            {
                userData["username"] = AppSession::instance().username();
            }
            userData["is_admin"] = AppSession::instance().isAdmin() ? 1 : 0;
        }
        emit profileUpdateSuccess(message, userData);
    }
    else
    {
        qWarning() << "收到未知的成功响应 action:" << action;
    }
}

void NetworkManager::reconnectToLastEndpoint()
{
    if (m_lastHost.isEmpty())
    {
        m_reconnectPending = false;
        return;
    }

    QTimer::singleShot(200, this, [this]()
                       {
        qInfo() << "重新连接服务器" << m_lastHost << ":" << m_lastPort;
        if (!m_socket) {
            m_reconnectPending = false;
            return;
        }
        m_reconnectPending = false;
        m_socket->connectToHost(m_lastHost, m_lastPort); });
}

void NetworkManager::onConnected()
{
    qInfo() << "已连接到服务器!";
    emit connected();
    m_receiveBuffer.clear();
    m_pendingRequests.clear();

    // 连接成功后自动发送tag注册
    QString tag = generateUniqueTag();
    m_clientTag = tag;
    sendTagRegistration(tag);
}
void NetworkManager::onDisconnected()
{
    qWarning() << "与服务器断开连接";
    m_tagRegistered = false; // 重置tag注册状态
    m_pendingRequests.clear();
    m_receiveBuffer.clear();
    m_clientTag.clear();
    emit disconnected();

    if (m_reconnectPending)
    {
        reconnectToLastEndpoint();
    }
}
void NetworkManager::onError(QAbstractSocket::SocketError socketError)
{
    // 这个 onError 主要处理底层的TCP错误 (比如连不上服务器)
    qCritical() << "网络底层错误:" << m_socket->errorString();
    emit generalError(m_socket->errorString());
}

// Tag注册功能实现
void NetworkManager::sendTagRegistration(const QString &tag)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState)
    {
        qWarning() << "未连接到服务器，无法注册tag";
        emit tagRegistrationFailed("未连接到服务器");
        return;
    }

    QJsonObject request;
    request["tag"] = tag;

    QJsonDocument doc(request);
    writeFramedJson(doc);
    qDebug() << "发送tag注册请求:" << tag;
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

// 发送JSON的通用函数
void NetworkManager::sendJsonRequest(const QJsonObject &request)
{
    QString actionName = request.value("action").toString();

    if (m_socket->state() != QAbstractSocket::ConnectedState)
    {
        qWarning() << "未连接到服务器，无法发送消息";
        emitActionFailed(actionName, "未连接到服务器");
        return;
    }

    if (!m_tagRegistered)
    {
        qWarning() << "Tag未注册，无法发送业务请求";
        emitActionFailed(actionName, "Tag未注册，请先完成tag注册");
        return;
    }

    QJsonDocument doc(request);
    writeFramedJson(doc);
    qDebug() << "发送JSON请求:" << request;

    if (!actionName.isEmpty())
    {
        PendingRequest pending{actionName, request.value("data").toObject()};
        m_pendingRequests.enqueue(pending);
    }
}

void NetworkManager::writeFramedJson(const QJsonDocument &document)
{
    QByteArray payload = document.toJson(QJsonDocument::Compact);

    quint32 length = static_cast<quint32>(payload.size());

    QByteArray frame;
    frame.resize(sizeof(quint32));
    qToBigEndian(length, reinterpret_cast<uchar *>(frame.data()));
    frame.append(payload);

    m_socket->write(frame);
}

void NetworkManager::emitActionFailed(const QString &action, const QString &message)
{
    if (action == "login")
    {
        emit loginFailed(message);
    }
    else if (action == "search_flights")
    {
        emit searchFailed(message);
    }
    else if (action == "register")
    {
        emit registerFailed(message);
    }
    else if (action == "book_flight")
    {
        emit bookingFailed(message);
    }
    else if (action == "get_my_orders")
    {
        emit myOrdersFailed(message);
    }
    else if (action == "cancel_order")
    {
        emit cancelOrderFailed(message);
    }
    else if (action == "update_profile")
    {
        emit profileUpdateFailed(message);
    }
    else if (!action.isEmpty())
    {
        emit generalError(message);
    }
    else
    {
        emit generalError(message);
    }
}

QJsonObject NetworkManager::detachPendingPayload(QString &action)
{
    QJsonObject payload;
    QQueue<PendingRequest> updated;
    bool removed = false;

    while (!m_pendingRequests.isEmpty())
    {
        PendingRequest current = m_pendingRequests.dequeue();
        if (!removed && (action.isEmpty() || current.action == action))
        {
            if (action.isEmpty())
            {
                action = current.action;
            }
            payload = current.payload;
            removed = true;
            continue;
        }
        updated.enqueue(current);
    }

    m_pendingRequests = updated;
    return payload;
}

// 构建各种请求 (给UI调用)

void NetworkManager::sendLoginRequest(const QString &username, const QString &password)
{
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
    QJsonObject data;
    data["user_id"] = userId;
    data["username"] = username;
    data["password"] = password;

    QJsonObject request;
    request["action"] = "update_profile";
    request["data"] = data;

    sendJsonRequest(request);
}

void NetworkManager::sendSearchRequest(const QString &origin, const QString &dest, const QString &date,
                                       const QString &cabinClass,
                                       const QStringList &passengerTypes)
{
    QJsonObject data;
    if (!origin.isEmpty())
        data["origin"] = origin;
    if (!dest.isEmpty())
        data["destination"] = dest;
    if (!date.isEmpty())
        data["date"] = date;
    if (!cabinClass.isEmpty())
        data["cabin_class"] = cabinClass;
    if (!passengerTypes.isEmpty())
    {
        data["passenger_types"] = QJsonArray::fromStringList(passengerTypes);
    }

    QJsonObject request;
    request["action"] = "search_flights";
    request["data"] = data;

    sendJsonRequest(request);
}

void NetworkManager::sendRegisterRequest(const QString &username, const QString &password)
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
