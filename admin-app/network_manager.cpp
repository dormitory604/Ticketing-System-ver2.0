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

    // socket发送
    QJsonDocument doc(identity);
    m_socket->write(doc.toJson(QJsonDocument::Compact));
    m_socket->flush();

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

    // 打包JSON
    QJsonDocument doc(json);
    QByteArray data = doc.toJson(QJsonDocument::Compact);  // Compact 表示压缩去空格
    // 投递，把二进制数据写入网线
    m_socket->write(data);
    // 强制刷新，确保数据立刻发出去，不要积压在缓存里
    m_socket->flush();

    qDebug() << "C2S 发送:" << json;
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

// 信息接收逻辑
void NetworkManager::onReadyRead()
{
    // 读取所有缓冲区数据
    QByteArray data = m_socket->readAll();
    // 把二进制还原为JSON格式
    QJsonDocument doc = QJsonDocument::fromJson(data);

    // 检查有没有无效数据
    if (doc.isNull() || !doc.isObject())
    {
        qWarning() << "收到无效数据:" << data;
        return;
    }

    // 提取通用信息
    QJsonObject root = doc.object();
    // 状态：是"success"还是"error"
    QString status = root["status"].toString();
    // 留言：服务器附带的一句话，比如"登录成功"或"密码错误"
    QString message = root["message"].toString();
    // 信息：用QJsonValue，因为它可能是个对象，也可能是个数组，还可能是空的
    QJsonValue rawData = root["data"];
    // 打印接收到的信息
    qDebug() << "S2C 收到:" << root;

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
    // 1. 判断是否是【登录成功】：特征: data是对象，且包含"is_admin"字段
    if (rawData.isObject() && rawData.toObject().contains("is_admin"))
    {
        // 把is_admin的值提取出来，是1则为管理员，是0则不是管理员
        bool isAdmin = (rawData.toObject()["is_admin"].toInt() == 1);
        // 通知登录窗口
        emit loginResult(true, isAdmin, message);
        return;
    }

    // 2. 判断是否是【列表数据】：航班、用户、订单
    if (rawData.isArray())
    {
        QJsonArray arr = rawData.toArray();

        // 如果数组为空，根据上一步的操作做出回应
        if (arr.isEmpty())
        {
            if (m_lastRequestType == FlightList) // 上一步是获取航班
            {
                emit allFlightsReceived(arr); // 发送空数组，界面会被清空
            }
            else if (m_lastRequestType == UserList) // 上一步是获取用户信息
            {
                emit allUsersReceived(arr);
            }
            else if (m_lastRequestType == BookingList) // 上一步是获取订单信息
            {
                emit allBookingsReceived(arr);
            }
        }
        else
        {
            // 拿出数组中的第一列，查看是什么列表
            QJsonObject firstItem = arr.first().toObject();


            // 航班列表特征: 有 "flight_number"
            if (firstItem.contains("flight_number"))
            {
                emit allFlightsReceived(arr);
            }
            // 用户列表特征: 有 "username" 但没有 "flight_number"
            else if (firstItem.contains("username"))
            {
                emit allUsersReceived(arr);
            }

            // 订单列表特征: 有 "booking_id"
            else if (firstItem.contains("booking_id"))
            {
                emit allBookingsReceived(arr);
            }
        }

        // 重置状态
        m_lastRequestType = None;
        return;
    }

    // 3. 判断是否是【操作成功】（添加、删除等）：特征: data是null或空对象，但status是success
    if (status == "success" && (rawData.isNull() || rawData.toObject().isEmpty()))
    {
        // 排除掉查询返回空数组的情况，这里主要指 INSERT/DELETE/UPDATE 的成功响应
        // 只有非查询类操作才会返回 null data (根据文档)
        emit adminOperationSuccess(message);
    }
}
