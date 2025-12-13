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
    m_requestQueue.append(FlightList); // 入队
    QJsonObject request;
    request["action"] = "admin_get_all_flights";
    request["data"] = QJsonObject(); // 这个接口不需要 data 参数

    send(request);
}

// 发送搜索请求
void NetworkManager::sendAdminSearchFlightsRequest(const QString& origin, const QString& dest, const QString& date)
{
    // 搜索返回的还是航班列表，所以入队 FlightList，这样界面收到数据后会自动调用 updateFlightTable
    m_requestQueue.append(FlightList);

    QJsonObject data;
    data["origin"] = origin;
    data["destination"] = dest;
    data["date"] = date;

    QJsonObject request;
    // 注意：Server端必须实现这个 "admin_search_flights" 的 action
    request["action"] = "admin_search_flights";
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
    m_requestQueue.append(UserList); // 入队
    QJsonObject request;
    request["action"] = "admin_get_all_users";  // 对应server-app中的管理员接口handleAdminGetAllUsers，表示这是获取所有用户
    request["data"] = QJsonObject(); // 空数据

    send(request);
}

// 获取所有订单(对应handleAdminGetAllBookings)
void NetworkManager::sendAdminGetAllBookingsRequest()
{
    m_requestQueue.append(BookingList); // 入队
    QJsonObject request;
    request["action"] = "admin_get_all_bookings";  // 对应server-app中的管理员接口handleAdminGetAllBookings，表示这是获取所有订单
    request["data"] = QJsonObject();

    send(request);
}

// 信息接收逻辑
void NetworkManager::onReadyRead()
{
    // 1. 将新收到的数据追加到缓冲区
    m_buffer.append(m_socket->readAll());

    // 2. 循环尝试解析缓冲区的数据
    // (这是为了处理粘包：一次收到多条 JSON，或者拆包：数据没收全)
    while (!m_buffer.isEmpty())
    {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(m_buffer, &parseError);

        if (parseError.error == QJsonParseError::NoError)
        {
            // 解析成功，说明缓冲区里包含了一条完整的 JSON
            if (doc.isObject())
            {
                processMessage(doc.object());
            }

            // 解析处理完这一条后，清空缓冲区
            m_buffer.clear();
        }
        else if (parseError.error == QJsonParseError::UnterminatedObject ||
                 parseError.error == QJsonParseError::UnterminatedArray ||
                 parseError.error == QJsonParseError::UnterminatedString)
        {
            break; // 继续等待后续数据
        }
        else
        {
            // 解析发生其他错误（可能是数据乱码），为防止程序卡死，清空缓冲区
            qWarning() << "JSON 格式错误，丢弃数据:" << parseError.errorString();
            m_buffer.clear();
            break;
        }
    }
}

// 提取出来的消息处理函数
void NetworkManager::processMessage(const QJsonObject& root)
{
    QString status = root["status"].toString();
    QString message = root["message"].toString();
    QJsonValue rawData = root["data"];

    qDebug() << "S2C 处理:" << root;

    // 1. 错误处理
    if (status == "error")
    {
        // 优先处理具体的登录错误
        if (message.contains("用户名或密码错误"))
        {
            emit loginResult(false, false, message);
        }
        else
        {
            // 其他通用错误（如数据库错误、权限不足等）
            emit adminOperationFailed(message);
        }

        // 如果请求失败了，必须把队列头部的 Pending 请求移除！
        // 否则下一个成功的请求会匹配到这个错误的类型上，导致错位。
        if (!m_requestQueue.isEmpty())
        {
            // 假设错误的响应对应的是最早发出的那个请求
            m_requestQueue.removeFirst();
            qDebug() << "错误发生，移除队列头部请求，剩余:" << m_requestQueue.size();
        }
        return;
    }

    // 2. 登录成功
    // 特征：data 是对象 且 包含 is_admin
    if (rawData.isObject() && rawData.toObject().contains("is_admin"))
    {
        bool isAdmin = (rawData.toObject()["is_admin"].toInt() == 1);
        emit loginResult(true, isAdmin, message);
        return;
    }

    // 3. 列表数据 (Array)
    // 凡是返回数组的，肯定是查询请求 (Get Flights/Users/Bookings)
    if (rawData.isArray())
    {
        QJsonArray arr = rawData.toArray();
        RequestType type = None;

        // A. 优先使用队列匹配
        if (!m_requestQueue.isEmpty())
        {
            type = m_requestQueue.takeFirst(); // 取出并移除
        }
        // B. 如果队列意外为空，尝试内容推断
        else
        {
            if (!arr.isEmpty())
            {
                QJsonObject first = arr.first().toObject();
                if (first.contains("flight_number")) type = FlightList;
                else if (first.contains("username")) type = UserList;
                else if (first.contains("booking_id")) type = BookingList;
            }
        }

        // 分发信号
        // 注意：即使 arr 为空，也要发送信号，以便界面清空表格
        if (type == FlightList) emit allFlightsReceived(arr);
        else if (type == UserList) emit allUsersReceived(arr);
        else if (type == BookingList) emit allBookingsReceived(arr);

        return; // 处理完列表后直接返回，不要往下走到 Case 4
    }

    // 4. 通用操作成功
    // 特征：status=success 且 data 不是数组 (通常是 null 或 空对象)
    // 适用于：添加/删除/修改 航班
    if (status == "success")
    {
        // 修复Bug：如果是 Tag 注册成功的消息，不要弹窗，直接忽略
        if (message == "Tag registered")
        {
            return;
        }

        // 只有真正的操作（添加、删除等）才弹窗
        emit adminOperationSuccess(message);
    }
}

// 发送搜索订单请求
void NetworkManager::sendAdminSearchBookingsRequest(const QString& bookingId, const QString& username)
{
    // 告诉接收端，接下来收到的数组是订单列表
    m_requestQueue.append(BookingList);

    QJsonObject data;
    data["booking_id"] = bookingId;
    data["username"] = username;

    QJsonObject request;
    // 对应 Server 端需要实现的 handleAdminSearchBookings
    request["action"] = "admin_search_bookings";
    request["data"] = data;

    send(request);
}

// 发送取消订单请求
void NetworkManager::sendAdminCancelBookingRequest(int bookingId)
{
    // 这是一个操作动作，不返回列表，所以不需要 m_requestQueue.append

    QJsonObject data;
    data["booking_id"] = bookingId;

    QJsonObject request;
    // 对应 Server 端需要实现的 handleAdminCancelBooking
    request["action"] = "admin_cancel_booking";
    request["data"] = data;

    send(request);
}
