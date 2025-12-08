#include "tcp_server.h"
#include <QSqlQuery>

/// 以下为服务器正常启动与处理连接的功能实现

TcpServer::TcpServer(QObject *parent) : QObject(parent)
{
    m_server = new QTcpServer(this);
    // 当有新客户端连接时，触发 onNewConnection
    connect(m_server, &QTcpServer::newConnection, this, &TcpServer::onNewConnection);
}

void TcpServer::startServer(quint16 port)
{
    if (m_server->listen(QHostAddress::Any, port)) {
        qInfo() << "服务器已启动，监听端口:" << port;
    } else {
        qCritical() << "服务器启动失败:" << m_server->errorString();
    }
}

// 处理新的客户端连接
void TcpServer::onNewConnection()
{
    QTcpSocket *clientSocket = m_server->nextPendingConnection(); // 获取客户端 Socket
    qInfo() << "新客户端连接:" << clientSocket->peerAddress().toString();

    // 利用Qt的信息与槽机制，在客户端连接后持续监听用户是否发了数据/断开连接
    connect(clientSocket, &QTcpSocket::readyRead, this, &TcpServer::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &TcpServer::onDisconnected);
    
    // 设置5秒超时，如果客户端没有发送tag，则断开连接
    QTimer::singleShot(5000, clientSocket, [clientSocket]() {
        if (clientSocket->state() == QTcpSocket::ConnectedState && !clientSocket->property("tag_registered").toBool()) {
            qWarning() << "客户端未在5秒内发送tag，断开连接:" << clientSocket->peerAddress().toString();
            clientSocket->disconnectFromHost();
        }
    });
}

// 处理客户端发送的数据
void TcpServer::onReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return; //TO DO

    QByteArray receivedData = socket->readAll();
    qDebug() << "收到原始数据:" << receivedData;

    // JSON解析
    QJsonDocument jsonDoc = QJsonDocument::fromJson(receivedData);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        qWarning() << "收到无效的JSON格式";

        // 返回错误消息
        QJsonObject error;
        error["status"] = "error";
        error["message"] = "Invalid JSON format";
        sendJsonResponse(socket, error);

        return;
    }

    QJsonObject request = jsonDoc.object();
    qDebug() << "解析JSON请求:" << request;

    // 首次解析tag
    if (clients[socket].tag.isEmpty())
    {
        if (!request.contains("tag") || !request["tag"].isString()) {

            QJsonObject error;
            error["status"] = "error";
            error["message"] = "Missing or invalid tag";

            sendJsonResponse(socket, error);
            socket->disconnectFromHost();
            return;
        }

        QString tag = request["tag"].toString();

        // 检查 tag 是否重复
        for (auto it = clients.begin(); it != clients.end(); ++it) {
            if (it.value().tag == tag && it.key() != socket) {

                QJsonObject error;
                error["status"] = "error";
                error["message"] = "Tag already in use";

                sendJsonResponse(socket, error);
                socket->disconnectFromHost();
                return;
            }
        }

        // 绑定 tag
        clients[socket].tag = tag;
        socket->setProperty("tag_registered", true);
        qInfo() << "客户端注册tag成功:" << tag;

        // 回复注册成功
        QJsonObject success;
        success["status"] = "success";
        success["message"] = "Tag registered";
        sendJsonResponse(socket, success);

        return;
    }

    // 解析正常业务
    QJsonObject response = handleRequest(request);
    sendJsonResponse(socket, response);
}

// 当客户端断开连接时
void TcpServer::onDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    if (clients.contains(socket)) {
        qInfo() << "客户端断开连接: " << clients[socket].tag;
        clients.remove(socket);
    }

    socket->deleteLater();
}

// 用来发送JSON响应的辅助函数，此处返回的是QByteArray
void TcpServer::sendJsonResponse(QTcpSocket* socket, const QJsonObject& response)
{
    QJsonDocument doc(response);
    QByteArray data = doc.toJson();
    socket->write(data);
    qDebug() << "发送JSON响应:" << response;
}

/// 以下为服务器具体业务需求功能实现

// 请求分发路由器
QJsonObject TcpServer::handleRequest(const QJsonObject& request)
{
    QString action = request["action"].toString();
    QJsonObject data = request["data"].toObject();

    if (action == "register") {
        return handleRegister(data);
    }
    if (action == "login") {
        return handleLogin(data);
    }
    if (action == "search_flights") {
        return handleSearchFlights(data);
    }
    if (action == "book_flight") {
        return handleBookFlight(data);
    }
    if (action == "get_my_orders") {
        return handleGetMyOrders(data);
    }
    if (action == "cancel_order") {
        return handleCancelOrder(data);
    }
    if (action == "admin_add_flight") {
        return handleAdminAddFlight(data);
    }
    if (action == "admin_delete_flight") {
        return handleAdminDeleteFlight(data);
    }
    if (action == "admin_update_flight") {
        return handleAdminUpdateFlight(data);
    }
    if (action == "admin_get_all_users") {
        return handleAdminGetAllUsers();
    }
    if (action == "admin_get_all_bookings") {
        return handleAdminGetAllBookings();
    }
    if (action == "admin_get_all_flights") {
        return handleAdminGetAllFlights();
    }

    // 如果后续还需要添加其他查询功能，按照下面的方式写
    // 记得一定要添加相对应的handle函数！！！
    // if (action == "admin_add_flight") {
    //     return handleAdminAddFlight(data);
    // }

    // 处理未知 action
    return {
        {"status", "error"},
        {"message", "未知的 action: " + action}
    };
}

// 处理注册
QJsonObject TcpServer::handleRegister(const QJsonObject& data)
{
    // 1. 从 JSON 中取出字段
    QString username = data.value("username").toString();
    QString password = data.value("password").toString();

    // 基本校验
    if (username.isEmpty() || password.isEmpty()) {
        return {
            {"status", "error"},
            {"message", "用户名或密码不能为空"},
            {"data", QJsonValue()}
        };
    }

    // 2. 准备 SQL
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("INSERT INTO User (username, password) VALUES (?, ?)");

    query.addBindValue(username);
    query.addBindValue(password);

    // 3. 执行 SQL
    if (!query.exec()) {

        // SQLite UNIQUE 约束错误码是 19
        if (query.lastError().nativeErrorCode() == "19") {
            return {
                {"status", "error"},
                {"message", QString("用户名 '%1' 已存在").arg(username)},
                {"data", QJsonValue()}
            };
        }

        // 其他数据库错误
        return {
            {"status", "error"},
            {"message", "数据库错误：" + query.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    // 4. 返回成功 JSON
    return {
        {"status", "success"},
        {"message", "注册成功"},
        {"data", QJsonValue()}    // 注册无需返回用户信息
    };
}

// 处理登录
QJsonObject TcpServer::handleLogin(const QJsonObject& data)
{
    QString username = data["username"].toString();
    QString password = data["password"].toString();

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT user_id, is_admin FROM User WHERE username = :user AND password = :pass");
    query.bindValue(":user", username);
    query.bindValue(":pass", password);

    if (!query.exec()) {
        return {
            {"status", "error"},
            {"message", "数据库查询失败: " + query.lastError().text()}
        };
    }

    if (query.next()) { // 找到用户信息
        return {
            {"status", "success"},
            {"message", "登录成功"},
            {"data", QJsonObject{
                         {"user_id", query.value("user_id").toInt()},
                         {"username", username},
                         {"is_admin", query.value("is_admin").toInt()}
                     }}
        };
    } else { // 没找到用户信息
        return {
            {"status", "error"},
            {"message", "用户名或密码错误"}
        };
    }
}

// 处理航班查询
QJsonObject TcpServer::handleSearchFlights(const QJsonObject& data)
{
    // 可以按1/2/3个条件搜索
    QString origin = data.value("origin").toString();
    QString dest = data.value("destination").toString();
    QString date = data.value("date").toString();

    // 构造动态SQL查询语句
    QString sql = "SELECT * FROM Flight WHERE 1=1 "; // 这里1=1就是确保WHERE为真，然后就可以在后面添加AND语句
    QList<QVariant> binds;

    if (!origin.isEmpty()) {
        sql += " AND origin = ? ";
        binds.append(origin);
    }
    if (!dest.isEmpty()) {
        sql += " AND destination = ? ";
        binds.append(dest);
    }
    if (!date.isEmpty()) {
        // 匹配 'YYYY-MM-DD'
        sql += " AND DATE(departure_time) = ? ";
        binds.append(date);
    }

    // 按时间排序
    sql += " ORDER BY departure_time ASC";

    // 这里执行查询
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(sql);
    for(const QVariant& v : binds) {
        query.addBindValue(v);
    }

    if (!query.exec()) {
        return { /* 数据库错误响应 */ };
    }

    QJsonArray flights;
    while(query.next()) {
        QJsonObject flight;
        flight["flight_id"] = query.value("flight_id").toInt();
        flight["flight_number"] = query.value("flight_number").toString();
        flight["origin"] = query.value("origin").toString();
        flight["destination"] = query.value("destination").toString();
        flight["departure_time"] = query.value("departure_time").toDateTime().toString(Qt::ISODate);
        flight["arrival_time"] = query.value("arrival_time").toDateTime().toString(Qt::ISODate);
        flight["price"] = query.value("price").toDouble();
        flight["remaining_seats"] = query.value("remaining_seats").toInt();
        flights.append(flight);
    }

    return {
        {"status", "success"},
        {"message", "查询成功"},
        {"data", flights} // 返回航班数组
    };
}

// 预定航班
QJsonObject TcpServer::handleBookFlight(const QJsonObject& data)
{
    int userId = data.value("user_id").toInt();
    int flightId = data.value("flight_id").toInt();

    if (userId <= 0 || flightId <= 0) {
        return {
            {"status", "error"},
            {"message", "参数错误"},
            {"data", QJsonValue()}
        };
    }

    QSqlDatabase db = DatabaseManager::instance().database();
    db.transaction();  // 开始事务

    // 1. 检查余票
    QSqlQuery q1(db);
    q1.prepare("SELECT remaining_seats FROM Flight WHERE flight_id = ?");
    q1.addBindValue(flightId);

    if (!q1.exec() || !q1.next()) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "航班不存在"},
            {"data", QJsonValue()}
        };
    }

    int seats = q1.value(0).toInt();
    if (seats <= 0) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "票已售罄"},
            {"data", QJsonValue()}
        };
    }

    // 2. 扣减余票
    QSqlQuery q2(db);
    q2.prepare("UPDATE Flight SET remaining_seats = remaining_seats - 1 WHERE flight_id = ?");
    q2.addBindValue(flightId);

    if (!q2.exec()) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "更新航班座位失败"},
            {"data", QJsonValue()}
        };
    }

    // 3. 创建订单
    QSqlQuery q3(db);
    q3.prepare("INSERT INTO Booking (user_id, flight_id, status) VALUES (?, ?, 'confirmed')");
    q3.addBindValue(userId);
    q3.addBindValue(flightId);

    if (!q3.exec()) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "创建订单失败：" + q3.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    // 获取新订单 ID
    int bookingId = q3.lastInsertId().toInt();

    // 4. 全部成功 → 提交事务
    db.commit();

    // 5. 返回成功 JSON
    QJsonObject info;
    info["booking_id"] = bookingId;
    info["user_id"] = userId;
    info["flight_id"] = flightId;
    info["status"] = "confirmed";

    return {
        {"status", "success"},
        {"message", "预订成功"},
        {"data", info}
    };
}

// 获取我的订单
QJsonObject TcpServer::handleGetMyOrders(const QJsonObject& data)
{
    int userId = data.value("user_id").toInt();

    if (userId <= 0) {
        return {
            {"status", "error"},
            {"message", "user_id 无效"},
            {"data", QJsonValue()}
        };
    }

    QSqlQuery query(DatabaseManager::instance().database());

    // JOIN Booking 和 Flight，返回订单 + 航班信息
    query.prepare(R"(
        SELECT
            Booking.booking_id,
            Booking.status,
            Booking.booking_time,
            Flight.flight_id,
            Flight.flight_number,
            Flight.origin,
            Flight.destination,
            Flight.departure_time,
            Flight.arrival_time
        FROM Booking
        JOIN Flight ON Booking.flight_id = Flight.flight_id
        WHERE Booking.user_id = ?
        ORDER BY Booking.booking_time DESC
    )");

    query.addBindValue(userId);

    if (!query.exec()) {
        return {
            {"status", "error"},
            {"message", "数据库查询失败：" + query.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    QJsonArray orders;

    while (query.next()) {
        QJsonObject obj;

        obj["booking_id"]     = query.value("booking_id").toInt();
        obj["status"]         = query.value("status").toString();
        obj["booking_time"]   = query.value("booking_time").toString();

        obj["flight_id"]      = query.value("flight_id").toInt();
        obj["flight_number"]  = query.value("flight_number").toString();
        obj["origin"]         = query.value("origin").toString();
        obj["destination"]    = query.value("destination").toString();
        obj["departure_time"] = query.value("departure_time").toString();
        obj["arrival_time"]   = query.value("arrival_time").toString();

        orders.append(obj);
    }

    return {
        {"status", "success"},
        {"message", "查询成功"},
        {"data", orders}
    };
}

// 取消订单
QJsonObject TcpServer::handleCancelOrder(const QJsonObject& data)
{
    int bookingId = data.value("booking_id").toInt();

    if (bookingId <= 0) {
        return {
            {"status", "error"},
            {"message", "booking_id 无效"},
            {"data", QJsonValue()}
        };
    }

    QSqlDatabase db = DatabaseManager::instance().database();
    db.transaction();  // 开始事务

    // 1. 查询订单信息
    QSqlQuery q1(db);
    q1.prepare("SELECT flight_id, status FROM Booking WHERE booking_id = ?");
    q1.addBindValue(bookingId);

    if (!q1.exec() || !q1.next()) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "订单不存在"},
            {"data", QJsonValue()}
        };
    }

    int flightId = q1.value("flight_id").toInt();
    QString status = q1.value("status").toString();

    // 2. 如果已经取消，直接返回
    if (status == "cancelled") {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "订单已取消，无需重复操作"},
            {"data", QJsonValue()}
        };
    }

    // 3. 更新订单状态为 cancelled
    QSqlQuery q2(db);
    q2.prepare("UPDATE Booking SET status = 'cancelled' WHERE booking_id = ?");
    q2.addBindValue(bookingId);

    if (!q2.exec()) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "订单状态更新失败"},
            {"data", QJsonValue()}
        };
    }

    // 4. 退回该航班的余票 remaining_seats + 1
    QSqlQuery q3(db);
    q3.prepare("UPDATE Flight SET remaining_seats = remaining_seats + 1 WHERE flight_id = ?");
    q3.addBindValue(flightId);

    if (!q3.exec()) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "余票回退失败"},
            {"data", QJsonValue()}
        };
    }

    // 5. 无错误，提交事务
    db.commit();

    return {
        {"status", "success"},
        {"message", "订单已成功取消"},
        {"data", QJsonValue()}
    };
}

// 管理员-增加航班
QJsonObject TcpServer::handleAdminAddFlight(const QJsonObject& data)
{
    QString flightNumber   = data.value("flight_number").toString();
    QString origin         = data.value("origin").toString();
    QString destination    = data.value("destination").toString();
    QString departureTime  = data.value("departure_time").toString();
    QString arrivalTime    = data.value("arrival_time").toString();
    double price           = data.value("price").toDouble();
    int seats              = data.value("seats").toInt();

    if (flightNumber.isEmpty() || origin.isEmpty() || destination.isEmpty() ||
        departureTime.isEmpty() || arrivalTime.isEmpty() || seats <= 0)
    {
        return {
            {"status", "error"},
            {"message", "参数不完整或无效"},
            {"data", QJsonValue()}
        };
    }

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(R"(
        INSERT INTO Flight (
            flight_number, origin, destination,
            departure_time, arrival_time,
            price, seats, remaining_seats
        ) VALUES (
            :flight_number, :origin, :destination,
            :departure_time, :arrival_time,
            :price, :seats, :remaining_seats
        )
    )");

    query.bindValue(":flight_number",   flightNumber);
    query.bindValue(":origin",         origin);
    query.bindValue(":destination",    destination);
    query.bindValue(":departure_time", departureTime);
    query.bindValue(":arrival_time",   arrivalTime);
    query.bindValue(":price",          price);
    query.bindValue(":seats",          seats);
    query.bindValue(":remaining_seats", seats);   // 新航班剩余座位 = 总座位

    if (!query.exec()) {
        return {
            {"status", "error"},
            {"message", "数据库插入失败：" + query.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    int newFlightId = query.lastInsertId().toInt();

    QJsonObject info;
    info["flight_id"]       = newFlightId;
    info["flight_number"]   = flightNumber;
    info["origin"]          = origin;
    info["destination"]     = destination;
    info["departure_time"]  = departureTime;
    info["arrival_time"]    = arrivalTime;
    info["price"]           = price;
    info["seats"]           = seats;
    info["remaining_seats"] = seats;

    return {
        {"status", "success"},
        {"message", "航班添加成功"},
        {"data", info}
    };
}

// 管理员-更新航班
QJsonObject TcpServer::handleAdminUpdateFlight(const QJsonObject& data)
{
    int flightId = data.value("flight_id").toInt();
    QString flightNumber   = data.value("flight_number").toString();
    QString origin         = data.value("origin").toString();
    QString destination    = data.value("destination").toString();
    QString departureTime  = data.value("departure_time").toString();
    QString arrivalTime    = data.value("arrival_time").toString();
    double price           = data.value("price").toDouble();
    int seats              = data.value("seats").toInt();   // 总座位数（新值）

    if (flightId <= 0 || flightNumber.isEmpty() || origin.isEmpty() ||
        destination.isEmpty() || departureTime.isEmpty() ||
        arrivalTime.isEmpty() || seats <= 0)
    {
        return {
            {"status", "error"},
            {"message", "参数不完整或无效"},
            {"data", QJsonValue()}
        };
    }

    QSqlDatabase db = DatabaseManager::instance().database();
    db.transaction();  // 🔥 开始事务

    // 1. 查询旧座位数和剩余座位数
    QSqlQuery q1(db);
    q1.prepare("SELECT seats, remaining_seats FROM Flight WHERE flight_id = :id");
    q1.bindValue(":id", flightId);

    if (!q1.exec() || !q1.next()) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "航班不存在"},
            {"data", QJsonValue()}
        };
    }

    int oldSeats = q1.value("seats").toInt();
    int oldRemaining = q1.value("remaining_seats").toInt();

    // 2. 计算新的 remaining_seats
    int newRemaining = oldRemaining + (seats - oldSeats);
    if (newRemaining < 0) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "剩余座位不能为负数，更新失败"},
            {"data", QJsonValue()}
        };
    }

    // 3. 更新航班信息
    QSqlQuery q2(db);
    q2.prepare(R"(
        UPDATE Flight SET
            flight_number   = :flight_number,
            origin          = :origin,
            destination     = :destination,
            departure_time  = :departure_time,
            arrival_time    = :arrival_time,
            price           = :price,
            seats           = :seats,
            remaining_seats = :remaining_seats
        WHERE flight_id = :flight_id
    )");

    q2.bindValue(":flight_number", flightNumber);
    q2.bindValue(":origin", origin);
    q2.bindValue(":destination", destination);
    q2.bindValue(":departure_time", departureTime);
    q2.bindValue(":arrival_time", arrivalTime);
    q2.bindValue(":price", price);
    q2.bindValue(":seats", seats);
    q2.bindValue(":remaining_seats", newRemaining);
    q2.bindValue(":flight_id", flightId);

    if (!q2.exec()) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "航班更新失败：" + q2.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    db.commit();

    // 返回成功信息和更新后的航班数据
    QJsonObject obj;
    obj["flight_id"]       = flightId;
    obj["flight_number"]   = flightNumber;
    obj["origin"]          = origin;
    obj["destination"]     = destination;
    obj["departure_time"]  = departureTime;
    obj["arrival_time"]    = arrivalTime;
    obj["price"]           = price;
    obj["seats"]           = seats;
    obj["remaining_seats"] = newRemaining;

    return {
        {"status", "success"},
        {"message", "航班更新成功"},
        {"data", obj}
    };
}

// 管理员-删除航班
QJsonObject TcpServer::handleAdminDeleteFlight(const QJsonObject& data)
{
    int flightId = data.value("flight_id").toInt();

    if (flightId <= 0) {
        return {
            {"status", "error"},
            {"message", "flight_id 无效"},
            {"data", QJsonValue()}
        };
    }

    QSqlDatabase db = DatabaseManager::instance().database();
    db.transaction();  // 🔥 开始事务

    // 1. 检查航班是否存在
    QSqlQuery q1(db);
    q1.prepare("SELECT flight_number FROM Flight WHERE flight_id = :id");
    q1.bindValue(":id", flightId);

    if (!q1.exec() || !q1.next()) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "航班不存在"},
            {"data", QJsonValue()}
        };
    }

    QString flightNumber = q1.value("flight_number").toString();

    // 2. 检查是否已有订单关联该航班
    QSqlQuery q2(db);
    q2.prepare("SELECT COUNT(*) FROM Booking WHERE flight_id = :id");
    q2.bindValue(":id", flightId);

    if (!q2.exec() || !q2.next()) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "数据库检查失败：" + q2.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    int bookingCount = q2.value(0).toInt();
    if (bookingCount > 0) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", QString("无法删除：该航班已有 %1 个订单").arg(bookingCount)},
            {"data", QJsonValue()}
        };
    }

    // 3. 删除航班
    QSqlQuery q3(db);
    q3.prepare("DELETE FROM Flight WHERE flight_id = :id");
    q3.bindValue(":id", flightId);

    if (!q3.exec()) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "航班删除失败：" + q3.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    db.commit();

    // 返回成功信息
    QJsonObject ret;
    ret["flight_id"]     = flightId;
    ret["flight_number"] = flightNumber;

    return {
        {"status", "success"},
        {"message", "航班删除成功"},
        {"data", ret}
    };
}

// 管理员-获取所有用户
QJsonObject TcpServer::handleAdminGetAllUsers()
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(R"(
        SELECT
            user_id, username, is_admin
        FROM User
        ORDER BY user_id ASC
    )");

    if (!query.exec()) {
        return {
            {"status", "error"},
            {"message", "查询用户失败：" + query.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    QJsonArray users;

    while (query.next()) {
        QJsonObject obj;
        obj["user_id"]  = query.value("user_id").toInt();
        obj["username"] = query.value("username").toString();
        obj["is_admin"] = query.value("is_admin").toInt(); // 0/1

        users.append(obj);
    }

    return {
        {"status", "success"},
        {"message", "查询用户成功"},
        {"data", users}
    };
}

// 管理员-获取所有订单（含航班信息）
QJsonObject TcpServer::handleAdminGetAllBookings()
{

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(R"(
        SELECT
            Booking.booking_id,
            Booking.user_id,
            User.username,
            Booking.flight_id,
            Flight.flight_number,
            Flight.origin,
            Flight.destination,
            Flight.departure_time,
            Flight.arrival_time,
            Booking.status,
            Booking.booking_time
        FROM Booking
        JOIN User   ON Booking.user_id  = User.user_id
        JOIN Flight ON Booking.flight_id = Flight.flight_id
        ORDER BY Booking.booking_time DESC
    )");

    if (!query.exec()) {
        return {
            {"status", "error"},
            {"message", "查询订单失败：" + query.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    QJsonArray bookings;

    while (query.next()) {
        QJsonObject obj;
        obj["booking_id"]     = query.value("booking_id").toInt();
        obj["user_id"]        = query.value("user_id").toInt();
        obj["username"]       = query.value("username").toString();
        obj["flight_id"]      = query.value("flight_id").toInt();
        obj["flight_number"]  = query.value("flight_number").toString();
        obj["origin"]         = query.value("origin").toString();
        obj["destination"]    = query.value("destination").toString();
        obj["departure_time"] = query.value("departure_time").toString();
        obj["arrival_time"]   = query.value("arrival_time").toString();
        obj["status"]         = query.value("status").toString();
        obj["booking_time"]   = query.value("booking_time").toString();

        bookings.append(obj);
    }

    return {
        {"status", "success"},
        {"message", "查询所有订单成功"},
        {"data", bookings}
    };
}

// 管理员-获取所有航班列表
QJsonObject TcpServer::handleAdminGetAllFlights()
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(R"(
        SELECT
            flight_id, flight_number, origin, destination,
            departure_time, arrival_time,
            price, seats, remaining_seats
        FROM Flight
        ORDER BY departure_time ASC
    )");

    if (!query.exec()) {
        return {
            {"status", "error"},
            {"message", "查询航班失败：" + query.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    QJsonArray flights;

    while (query.next()) {
        QJsonObject obj;
        obj["flight_id"]       = query.value("flight_id").toInt();
        obj["flight_number"]   = query.value("flight_number").toString();
        obj["origin"]          = query.value("origin").toString();
        obj["destination"]     = query.value("destination").toString();
        obj["departure_time"]  = query.value("departure_time").toString();
        obj["arrival_time"]    = query.value("arrival_time").toString();
        obj["price"]           = query.value("price").toDouble();
        obj["seats"]           = query.value("seats").toInt();
        obj["remaining_seats"] = query.value("remaining_seats").toInt();

        flights.append(obj);
    }

    return {
        {"status", "success"},
        {"message", "查询所有航班成功"},
        {"data", flights}
    };
}


