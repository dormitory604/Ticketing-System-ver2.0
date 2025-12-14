#include "network_manager.h"
#include <QDebug>
#include <QRandomGenerator>

// 构造函数
NetworkManager::NetworkManager(QObject *parent) : QObject(parent)
{
    // 实例化TCP Socket，"this" 表示这个 Socket 归 NetworkManager 管理
    m_socket = new QTcpSocket(this);

    // 信号槽连接
    // socket发出readyRead信号，立刻执行onReadyRead函数
    connect(m_socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    // socket连接成功，执行onConnected函数
    connect(m_socket, &QTcpSocket::connected, this, &NetworkManager::onConnected);
    // socket断开，执行onDisconnected函数
    connect(m_socket, &QTcpSocket::disconnected, this, &NetworkManager::onDisconnected);

    // 构造时自动尝试连接
    connectToServer();
}

NetworkManager::~NetworkManager()
{
    if (m_socket->isOpen())
        m_socket->close();
}

// 尝试连接
void NetworkManager::connectToServer()
{
    // 检查状态：只有在没连上的时候才去连
    if (m_socket->state() == QAbstractSocket::UnconnectedState)
    {
        qDebug() << "Connecting to server..." << SERVER_IP << ":" << SERVER_PORT;
        m_socket->connectToHost(SERVER_IP, SERVER_PORT);
    }
}

// 判断现在socket是否连接（只读）
bool NetworkManager::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

// 打印连接成功
void NetworkManager::onConnected()
{
    qDebug() << "Server connection successful！";

    // 连接成功，立刻发送身份注册包
    QJsonObject identity;
    // 生成一个随机后缀，防止tag重复
    QString uniqueTag = "admin_" + QString::number(QRandomGenerator::global()->bounded(1000, 9999));

    identity["tag"] = uniqueTag;

    // 使用新的定长 send 函数发送身份注册包
    send(identity);

    qDebug() << "C2S 发送身份认证:" << identity;
}

// 打印断开连接
void NetworkManager::onDisconnected()
{
    qDebug() << "Server disconnected.";
}

// 辅助发送函数：负责打包JSON并写入Socket
void NetworkManager::send(const QJsonObject& json)
{
    // 检查有没有连接上服务器
    if (!isConnected())
    {
        qWarning() << "Send failed: Unable to connect to server.";
        emit adminOperationFailed("Unable to connect to server.");
        // 尝试重连
        connectToServer();
        return;
    }

    // 将 JSON 对象转为 Compact 格式的 QByteArray (Payload)
    QJsonDocument doc(json);
    QByteArray payload = doc.toJson(QJsonDocument::Compact);

    // 计算长度并转换为 4 字节大端序 (Big Endian)
    quint32 len = payload.size();
    QByteArray block;
    block.resize(sizeof(quint32));
    // 使用 QtEndian 确保字节序与服务器端一致 (服务器是 Big Endian)
    qToBigEndian(len, reinterpret_cast<uchar*>(block.data()));

    // 拼接 [4字节长度] + [JSON 字节]
    block.append(payload);

    // 发送
    m_socket->write(block);
    m_socket->flush();

    qDebug() << "C2S 发送 (len:" << len << "):" << json;
}

// 登录请求
void NetworkManager::sendAdminLoginRequest(const QString& username, const QString& password)
{
    // 填写数据
    QJsonObject data;
    data["username"] = username;
    data["password"] = password;

    QJsonObject request;
    request["action"] = "login";  // 对应server-app中的handleLogin，表示这是登录操作
    request["data"] = data;

    send(request);
}

// 获取航班
void NetworkManager::sendGetAllFlightsRequest()
{
    m_lastRequestType = FlightList;  //记录：上一步的操作是获取航班
    QJsonObject request;
    // 使用 admin 接口获取所有航班（已在服务端限制1000条）
    request["action"] = "admin_get_all_flights";
    request["data"] = QJsonObject(); // 这个接口不需要 data 参数

    send(request);
}

// 航班搜索请求 (调用 handleSearchFlights 接口)
void NetworkManager::sendSearchFlightsRequest(const QString& origin, const QString& destination, const QString& date)
{
    m_lastRequestType = FlightList;  //记录：上一步的操作是获取航班

    QJsonObject data;

    // 仅添加非空字段，让服务器动态构建 SQL
    if (!origin.isEmpty()) {
        data["origin"] = origin;
    }
    if (!destination.isEmpty()) {
        data["destination"] = destination;
    }
    // 假设日期总是有效且不为空（由 QDateEdit 保证）
    if (!date.isEmpty()) {
        data["date"] = date;
    }


    QJsonObject request;
    request["action"] = "search_flights"; // 使用客户端的通用搜索接口
    request["data"] = data;

    send(request);
}

// 添加航班(对应handleAdminAddFlight)
void NetworkManager::sendAdminAddFlightRequest(const QJsonObject& flightData)
{
    QJsonObject request;
    request["action"] = "admin_add_flight";  // 对应server-app中的管理员接口handleAdminAddFlight，表示这是添加航班
    request["data"] = flightData;

    send(request);
}

// 删除航班(对应handleAdminDeleteFlight)
void NetworkManager::sendAdminDeleteFlightRequest(int flightId)
{
    QJsonObject data;
    data["flight_id"] = flightId;

    QJsonObject request;
    request["action"] = "admin_delete_flight";  // 对应server-app中的管理员接口handleAdminDeleteFlight，表示这是删除航班
    request["data"] = data;

    send(request);
}

// 修改航班(对应handleAdminUpdateFlight)
void NetworkManager::sendAdminUpdateFlightRequest(int flightId, const QJsonObject& changes)
{
    // 把flightId和要修改的字段合并到一个data对象里
    QJsonObject data = changes;
    data["flight_id"] = flightId;  // 告诉服务器要改哪一个航班

    QJsonObject request;
    request["action"] = "admin_update_flight";  // 对应server-app中的管理员接口handleAdminUpdateFlight，表示这是修改航班
    request["data"] = data;

    send(request);
}

// 获取所有用户(对应handleAdminGetAllUsers)
void NetworkManager::sendAdminGetAllUsersRequest()
{
    m_lastRequestType = UserList; //记录：上一步的操作是获取所有用户
    QJsonObject request;
    request["action"] = "admin_get_all_users";  // 对应server-app中的管理员接口handleAdminGetAllUsers，表示这是获取所有用户
    request["data"] = QJsonObject(); // 空数据

    send(request);
}

// 获取所有订单(对应handleAdminGetAllBookings)
void NetworkManager::sendAdminGetAllBookingsRequest()
{
    m_lastRequestType = BookingList;  // 记录：上一步的操作是获取所有订单
    QJsonObject request;
    request["action"] = "admin_get_all_bookings";  // 对应server-app中的管理员接口handleAdminGetAllBookings，表示这是获取所有订单
    request["data"] = QJsonObject();

    send(request);
}

// 拆分出的 JSON 处理逻辑
void NetworkManager::processJsonResponse(const QJsonObject& response)
{
    QString status = response["status"].toString();
    QString message = response["message"].toString();
    QJsonValue rawData = response["data"];

    // 打印接收到的信息 (在这里打印，而不是在 onReadyRead)
    qDebug() << "S2C 收到完整 JSON:" << response;

    // 错误处理
    if (status == "error")
    {
        // 如果是登录失败，发送登录失败信号
        if (message.contains("用户名或密码错误"))
        {
            emit loginResult(false, false, message);
        }
        else
        {
            // 其他通用错误，谁连接这个信号，谁就弹窗报错
            emit adminOperationFailed(message);
        }
        return;
    }

    // 检查接收到的信息是什么信息
    // 1. 判断是否是【登录成功】
    if (rawData.isObject() && rawData.toObject().contains("is_admin"))
    {
        bool isAdmin = (rawData.toObject()["is_admin"].toInt() == 1);
        emit loginResult(true, isAdmin, message);
        return;
    }

    // 2. 判断是否是【列表数据】：航班、用户、订单
    if (rawData.isArray())
    {
        QJsonArray arr = rawData.toArray();
        QJsonObject firstItem;
        if (!arr.isEmpty()) {
            firstItem = arr.first().toObject();
        }

        // [修改 A: 订单数据应优先于包含 username 的用户数据]
        // 订单列表特征: 有 "booking_id" (这是最独特的字段)
        if (m_lastRequestType == BookingList || firstItem.contains("booking_id"))
        {
            emit allBookingsReceived(arr);
            m_lastRequestType = None; // 立即重置
            return;
        }

        // 航班列表特征: 有 "flight_number"
        if (m_lastRequestType == FlightList || firstItem.contains("flight_number"))
        {
            emit allFlightsReceived(arr);
            m_lastRequestType = None; // 立即重置
            return;
        }

        // 用户列表特征: 有 "username" (放在最后，避免误判)
        if (m_lastRequestType == UserList || firstItem.contains("username"))
        {
            emit allUsersReceived(arr);
            m_lastRequestType = None; // 立即重置
            return;
        }

        // 如果都不是，但收到了数组，说明是未知的列表，不处理但重置状态
        if (!arr.isEmpty()) {
            qWarning() << "收到未知类型的非空数组响应";
            m_lastRequestType = None;
            return;
        }

        // 收到空数组，依赖 m_lastRequestType
        if (m_lastRequestType == FlightList)
        {
            emit allFlightsReceived(arr);
        }
        else if (m_lastRequestType == UserList)
        {
            emit allUsersReceived(arr);
        }
        else if (m_lastRequestType == BookingList)
        {
            emit allBookingsReceived(arr);
        }

        // 重置状态
        m_lastRequestType = None;
        return;
    }

    // 3. 判断是否是【操作成功】（添加、删除等）
    if (status == "success" && (rawData.isNull() || rawData.toObject().isEmpty()))
    {
        emit adminOperationSuccess(message);
    }
}

// 信息接收逻辑 - 必须实现定长包处理
void NetworkManager::onReadyRead()
{
    // 1. 读取所有缓冲区数据并追加到 m_readBuffer
    m_readBuffer.append(m_socket->readAll());

    // 2. 循环解析缓冲区中的所有完整消息
    while (true)
    {
        // A. 尝试读取长度前缀 (4字节)
        if (m_expectedLen == 0)
        {
            // 如果缓冲区中数据不够 4 字节，等待更多数据
            if (m_readBuffer.size() < static_cast<int>(sizeof(quint32)))
                break;

            // 读取 4 字节长度，并转换为本地字节序（服务器端是 Big Endian）
            quint32 len = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(m_readBuffer.constData()));

            // 设置期望长度
            m_expectedLen = len;

            // 从缓冲区移除已读取的 4 字节长度前缀
            m_readBuffer.remove(0, sizeof(quint32));
        }

        // B. 检查消息体是否完整
        if (m_readBuffer.size() < static_cast<int>(m_expectedLen))
        {
            // 数据不够一帧，等待更多数据
            break;
        }

        // C. 取出完整帧 (消息体)
        QByteArray message = m_readBuffer.left(static_cast<int>(m_expectedLen));

        // D. 从缓冲区移除已处理的帧，并重置期望长度，准备接收下一帧
        m_readBuffer.remove(0, static_cast<int>(m_expectedLen));
        m_expectedLen = 0;

        // E. 解析 JSON
        QJsonDocument doc = QJsonDocument::fromJson(message);

        // 检查是否有解析错误
        if (doc.isNull() || !doc.isObject())
        {
            qWarning() << "收到无效的 JSON 格式或数据损坏，消息体：" << message;
            continue; // 跳过此帧，继续处理下一帧
        }

        // F. 调用 JSON 处理逻辑
        processJsonResponse(doc.object());
    }
}
