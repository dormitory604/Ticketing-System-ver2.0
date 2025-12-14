#include "network_manager.h"

#include <QTimer>
#include <QDateTime>
#include <QRandomGenerator>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent), m_tagRegistered(false), m_expectedLen(0)
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
    qInfo() << "连接到服务器" << host << ":" << port;
    m_socket->connectToHost(host, port);
}

// 当收到服务器数据时 (长度前缀分帧解析)
void NetworkManager::onReadyRead()
{
    // 追加收到的数据到缓冲区
    m_recvBuf.append(m_socket->readAll());

    // 循环解析，可能一次解析多条消息
    while (true)
    {
        // 还没读到长度前缀
        if (m_expectedLen == 0)
        {
            if (m_recvBuf.size() < static_cast<int>(sizeof(quint32)))
                break;
            quint32 len = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(m_recvBuf.constData()));
            m_expectedLen = len;
            m_recvBuf.remove(0, sizeof(quint32));
        }

        // 数据不够一帧
        if (m_recvBuf.size() < static_cast<int>(m_expectedLen))
            break;

        // 取出完整帧
        QByteArray frame = m_recvBuf.left(m_expectedLen);
        m_recvBuf.remove(0, m_expectedLen);
        m_expectedLen = 0;

        qDebug() << "收到完整帧:" << frame;

        // 处理这一帧
        processFrame(frame);
    }
}

// 处理单个完整的JSON帧
void NetworkManager::processFrame(const QByteArray &frame)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(frame);
    if (jsonDoc.isNull() || !jsonDoc.isObject())
    {
        qWarning() << "收到无效的JSON格式";
        emit generalError("收到无效的服务器响应 (非JSON)");
        return;
    }

    QJsonObject response = jsonDoc.object();
    QString status = response["status"].toString();
    QString message = response["message"].toString();

    // 检查是否是tag注册响应（没有action_response字段）
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

    QString action = response["action_response"].toString();

    if (!action.isEmpty() && !m_pendingActions.isEmpty() && m_pendingActions.head() == action)
    {
        // 丢弃已匹配的动作，保持队列与响应同步
        m_pendingActions.dequeue();
    }

    if (action.isEmpty())
    {
        if (response.contains("action"))
        {
            action = response["action"].toString();
        }
        // 如果服务器没有返回action字段，则退回到本地记录的请求顺序
        if (action.isEmpty() && !m_pendingActions.isEmpty())
        {
            action = m_pendingActions.dequeue();
        }
    }

    // 路由 "失败" 响应
    if (status == "error")
    {
        qWarning() << "服务器返回错误:" << message << " (Action: " << action << ")";

        if (action == "login")
        {
            emit loginFailed(message);
        }
        else if (action == "search_flights")
        {
            emit generalError(message);
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
            emit generalError(message);
        }
        else if (action == "cancel_order")
        {
            emit cancelOrderFailed(message);
        }
        else if (action == "update_profile")
        {
            emit profileUpdateFailed(message);
        }
        else
        {
            emit generalError(message);
        }
        return;
    }

    // 路由 "成功" 响应
    // action字段一定不能为空！
    if (action.isEmpty())
    {
        return;
    }

    // 开始路由
    if (action == "login")
    {
        emit loginSuccess(response["data"].toObject());
    }
    else if (action == "search_flights")
    {
        emit searchResults(response["data"].toArray());
    }
    else if (action == "register")
    {
        emit registerSuccess(message);
    }
    else if (action == "book_flight")
    {
        emit bookingSuccess(response["data"].toObject());
    }
    else if (action == "get_my_orders")
    {
        emit myOrdersResult(response["data"].toArray());
    }
    else if (action == "cancel_order")
    {
        emit cancelOrderSuccess(message);
    }
    else if (action == "update_profile")
    {
        emit profileUpdateSuccess(message, response["data"].toObject());
    }
    /*
    else if (action == "admin_add_flight") {
        emit adminOpSuccess(message);
    }
    */
    else
    {
        // 收到一个服务器认识、但客户端不认识的 action
        qWarning() << "收到未知的成功响应 action: " << action;
    }
}

void NetworkManager::onConnected()
{
    qInfo() << "已连接到服务器!";
    emit connected();

    // 连接成功后自动发送tag注册
    QString tag = generateUniqueTag();
    m_clientTag = tag;
    sendTagRegistration(tag);
}
void NetworkManager::onDisconnected()
{
    qWarning() << "与服务器断开连接";
    m_tagRegistered = false; // 重置tag注册状态
    emit disconnected();
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
    QByteArray payload = doc.toJson(QJsonDocument::Compact);

    // 添加长度前缀
    quint32 len = payload.size();
    QByteArray block;
    block.resize(sizeof(quint32));
    qToBigEndian(len, reinterpret_cast<uchar *>(block.data()));
    block.append(payload);

    m_socket->write(block);
    qDebug() << "发送tag注册请求(带长度前缀):" << tag << "长度:" << len;
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
    if (m_socket->state() != QAbstractSocket::ConnectedState)
    {
        qWarning() << "未连接到服务器，无法发送消息";
        emit generalError("未连接到服务器");
        return;
    }

    if (!m_tagRegistered)
    {
        qWarning() << "Tag未注册，无法发送业务请求";
        emit generalError("Tag未注册，请先完成tag注册");
        return;
    }

    QString actionName = request.value("action").toString();

    QJsonDocument doc(request);
    QByteArray payload = doc.toJson(QJsonDocument::Compact);

    // 添加长度前缀：[4字节大端长度][JSON字节流]
    quint32 len = payload.size();
    QByteArray block;
    block.resize(sizeof(quint32));
    qToBigEndian(len, reinterpret_cast<uchar *>(block.data()));
    block.append(payload);

    m_socket->write(block);
    qDebug() << "发送JSON请求(带长度前缀):" << request << "长度:" << len;

    if (!actionName.isEmpty())
    {
        m_pendingActions.enqueue(actionName);
    }
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
