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
    if (!socket) return;

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
    if (action == "update_profile") {
        return handleUpdateProfile(data);
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

// 更新用户信息
QJsonObject TcpServer::handleUpdateProfile(const QJsonObject &data)
{
    int userId = data.value("user_id").toInt();
    QString username = data.value("username").toString();
    QString password = data.value("password").toString();

    if (userId <= 0) {
        return {
            {"status", "error"},
            {"message", "user_id 无效"},
            {"data", QJsonValue()}
        };
    }

    // 动态构造 SQL
    QString sql = "UPDATE User SET ";
    QList<QString> sets;
    QList<QVariant> binds;

    if (!username.isEmpty()) {
        sets.append("username = ?");
        binds.append(username);
    }
    if (!password.isEmpty()) {
        sets.append("password = ?");
        binds.append(password);
    }

    if (sets.isEmpty()) {
        return {
            {"status", "error"},
            {"message", "无任何可更新字段"},
            {"data", QJsonValue()}
        };
    }

    sql += sets.join(", ");
    sql += " WHERE user_id = ?";

    binds.append(userId);

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(sql);

    for (const QVariant& v : binds)
        query.addBindValue(v);

    if (!query.exec()) {
        // 处理 UNIQUE 冲突 (用户名已存在)
        if (query.lastError().nativeErrorCode() == "19") {
            return {
                {"status", "error"},
                {"message", QString("用户名 '%1' 已存在").arg(username)},
                {"data", QJsonValue()}
            };
        }

        return {
            {"status", "error"},
            {"message", "数据库更新失败：" + query.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    return {
        {"status", "success"},
        {"message", "用户资料更新成功"},
        {"data", QJsonValue()}
    };
}

// 处理航班查询
QJsonObject TcpServer::handleSearchFlights(const QJsonObject& data)
{
    QString origin      = data.value("origin").toString();
    QString destination = data.value("destination").toString();
    QString date        = data.value("date").toString();     // YYYY-MM-DD

    // 构建 SQL
    QString sql = R"(
        SELECT flight_id, flight_number, model, origin, destination,
               departure_time, arrival_time, price, remaining_seats
        FROM Flight
    )";

    QStringList where;
    QMap<QString, QVariant> binds;

    where << "is_deleted = 0";

    if (!origin.isEmpty()) {
        where << "origin = :origin";
        binds[":origin"] = origin;
    }
    if (!destination.isEmpty()) {
        where << "destination = :destination";
        binds[":destination"] = destination;
    }
    if (!date.isEmpty()) {
        // departure_time LIKE '2025-12-05%'
        where << "departure_time LIKE :date";
        binds[":date"] = date + "%";
    }

    if (!where.isEmpty()) {
        sql += " WHERE " + where.join(" AND ");
    }

    sql += " ORDER BY departure_time ASC LIMIT :limit";

    QSqlQuery query(DatabaseManager::instance().database());
    if (!query.prepare(sql)) {
        return {
            {"status", "error"},
            {"message", "数据库 prepare 失败：" + query.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    for (auto it = binds.begin(); it != binds.end(); ++it) {
        query.bindValue(it.key(), it.value());
    }

    query.bindValue(":limit", MAX_RETURN_ROWS);

    if (!query.exec()) {
        return {
            {"status", "error"},
            {"message", "数据库查询失败：" + query.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    QJsonArray flights;

    while (query.next()) {
        QJsonObject f;
        f["flight_id"]       = query.value("flight_id").toInt();
        f["flight_number"]   = query.value("flight_number").toString();
        f["model"]           = query.value("model").toString();
        f["origin"]          = query.value("origin").toString();
        f["destination"]     = query.value("destination").toString();
        f["departure_time"]  = query.value("departure_time").toString();
        f["arrival_time"]    = query.value("arrival_time").toString();
        f["price"]           = query.value("price").toDouble();
        f["remaining_seats"] = query.value("remaining_seats").toInt();

        flights.append(f);
    }

    return {
        {"status", "success"},
        {"message", "查询成功"},
        {"data", flights}
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
            {"message", "user_id 或 flight_id 无效"},
            {"data", QJsonValue()}
        };
    }

    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.transaction()) {
        return {
            {"status", "error"},
            {"message", "无法开启事务：" + db.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    // 1. 检查航班是否存在并读取剩余座位
    QSqlQuery q1(db);
    q1.prepare(R"(
        SELECT remaining_seats
        FROM Flight
        WHERE flight_id = :flight_id
    )");
    q1.bindValue(":flight_id", flightId);

    if (!q1.exec() || !q1.next()) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "航班不存在或查询失败：" + q1.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    int remaining = q1.value("remaining_seats").toInt();
    if (remaining <= 0) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "票已售罄"},
            {"data", QJsonValue()}
        };
    }

    // 2. 扣减剩余座位（并发安全：必须 remaining_seats > 0 才扣）
    QSqlQuery q2(db);
    q2.prepare(R"(
        UPDATE Flight
        SET remaining_seats = remaining_seats - 1
        WHERE flight_id = :flight_id
          AND remaining_seats > 0
    )");
    q2.bindValue(":flight_id", flightId);

    if (!q2.exec()) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "扣减座位失败：" + q2.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    if (q2.numRowsAffected() == 0) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "票已售罄"},
            {"data", QJsonValue()}
        };
    }

    // 3. 插入订单（状态：confirmed）
    QSqlQuery q3(db);
    q3.prepare(R"(
        INSERT INTO Booking (user_id, flight_id, status)
        VALUES (:user_id, :flight_id, 'confirmed')
    )");
    q3.bindValue(":user_id", userId);
    q3.bindValue(":flight_id", flightId);

    if (!q3.exec()) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "订单创建失败：" + q3.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    int bookingId = q3.lastInsertId().toInt();

    if (!db.commit()) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "提交事务失败：" + db.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    // 返回订单基础信息
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

    if (!query.prepare(R"(
        SELECT
            b.booking_id,
            b.flight_id,
            b.status,
            b.booking_time,
            f.flight_number,
            f.origin,
            f.destination,
            f.departure_time,
            f.arrival_time,
            f.price,
            f.is_deleted
        FROM Booking b
        JOIN Flight f ON b.flight_id = f.flight_id
        WHERE b.user_id = :user_id
        ORDER BY b.booking_time DESC
        LIMIT :limit
    )"))
    {
        return {
            {"status", "error"},
            {"message", "查询失败：" + query.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    query.bindValue(":user_id", userId);
    query.bindValue(":limit", MAX_RETURN_ROWS);

    if (!query.exec()) {
        return {
            {"status", "error"},
            {"message", "查询失败：" + query.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    QJsonArray arr;

    while (query.next()) {
        QJsonObject item;

        item["booking_id"]     = query.value("booking_id").toInt();
        item["flight_id"]      = query.value("flight_id").toInt();
        item["status"]         = query.value("status").toString();
        item["flight_number"]  = query.value("flight_number").toString();
        item["origin"]         = query.value("origin").toString();
        item["destination"]    = query.value("destination").toString();
        item["departure_time"] = query.value("departure_time").toString();
        item["is_deleted"]     = query.value("is_deleted").toInt();


        arr.append(item);
    }

    return {
        {"status", "success"},
        {"message", "查询成功"},
        {"data", arr}
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
    if (!db.transaction()) {
        return {
            {"status", "error"},
            {"message", "无法开启事务：" + db.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    // 1. 查询订单状态与 flight_id
    QSqlQuery q1(db);
    q1.prepare(R"(
        SELECT flight_id, status
        FROM Booking
        WHERE booking_id = :booking_id
    )");
    q1.bindValue(":booking_id", bookingId);

    if (!q1.exec()) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "订单查询失败：" + q1.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    if (!q1.next()) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "订单不存在"},
            {"data", QJsonValue()}
        };
    }

    int flightId = q1.value("flight_id").toInt();
    QString status = q1.value("status").toString();

    // 已取消不可重复取消
    if (status == "cancelled") {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "订单已取消"},
            {"data", QJsonValue()}
        };
    }

    // 2. 将订单状态改为取消
    QSqlQuery q2(db);
    q2.prepare(R"(
        UPDATE Booking
        SET status = 'cancelled'
        WHERE booking_id = :booking_id
    )");
    q2.bindValue(":booking_id", bookingId);

    if (!q2.exec()) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "更新订单状态失败：" + q2.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    // 3. 恢复航班剩余座位
    QSqlQuery q3(db);
    q3.prepare(R"(
        UPDATE Flight
        SET remaining_seats = remaining_seats + 1
        WHERE flight_id = :flight_id
    )");
    q3.bindValue(":flight_id", flightId);

    if (!q3.exec()) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "恢复座位失败：" + q3.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    if (!db.commit()) {
        db.rollback();
        return {
            {"status", "error"},
            {"message", "提交事务失败：" + db.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    return {
        {"status", "success"},
        {"message", "订单取消成功"},
        {"data", QJsonValue()}
    };
}

// 管理员-增加航班
QJsonObject TcpServer::handleAdminAddFlight(const QJsonObject& data)
{
    QString flightNumber   = data.value("flight_number").toString();
    QString model          = data.value("model").toString();
    QString origin         = data.value("origin").toString();
    QString destination    = data.value("destination").toString();
    QString departureTime  = data.value("departure_time").toString();
    QString arrivalTime    = data.value("arrival_time").toString();
    double price           = data.value("price").toDouble();
    int totalSeats         = data.value("total_seats").toInt();

    // 参数校验
    if (flightNumber.isEmpty() ||
        origin.isEmpty() || destination.isEmpty() ||
        departureTime.isEmpty() || arrivalTime.isEmpty() ||
        totalSeats <= 0 || price <= 0)
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
            flight_number, model, origin, destination,
            departure_time, arrival_time,
            total_seats, remaining_seats, price
        ) VALUES (
            :flight_number, :model, :origin, :destination,
            :departure_time, :arrival_time,
            :total_seats, :remaining_seats, :price
        )
    )");

    query.bindValue(":flight_number",   flightNumber);
    query.bindValue(":model",           model);
    query.bindValue(":origin",          origin);
    query.bindValue(":destination",     destination);
    query.bindValue(":departure_time",  departureTime);
    query.bindValue(":arrival_time",    arrivalTime);
    query.bindValue(":total_seats",     totalSeats);
    query.bindValue(":remaining_seats", totalSeats);
    query.bindValue(":price",           price);

    if (!query.exec()) {
        return {
            {"status", "error"},
            {"message", "数据库插入失败：" + query.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    return {
        {"status", "success"},
        {"message", "航班添加成功"},
        {"data", QJsonValue()}
    };
}


// 管理员-更新航班
QJsonObject TcpServer::handleAdminUpdateFlight(const QJsonObject& data)
{
    int flightId         = data.value("flight_id").toInt();
    QString flightNumber = data.value("flight_number").toString();
    QString origin       = data.value("origin").toString();
    QString destination  = data.value("destination").toString();
    QString departureTime= data.value("departure_time").toString();
    QString arrivalTime  = data.value("arrival_time").toString();
    double price         = data.value("price").toDouble();
    int totalSeats       = data.value("total_seats").toInt();     // FIX: seats → total_seats
    int remainingSeats   = data.value("remaining_seats").toInt();

    // 参数检查
    if (flightId <= 0 ||
        flightNumber.isEmpty() || origin.isEmpty() || destination.isEmpty() ||
        departureTime.isEmpty() || arrivalTime.isEmpty() ||
        totalSeats <= 0 || remainingSeats < 0)
    {
        return {
            {"status", "error"},
            {"message", "参数不完整或无效"},
            {"data", QJsonValue()}
        };
    }

    QSqlQuery q1(DatabaseManager::instance().database());
    q1.prepare(R"(
        SELECT total_seats, remaining_seats
        FROM Flight
        WHERE flight_id = :flight_id
    )");    // FIX: seats → total_seats

    q1.bindValue(":flight_id", flightId);

    if (!q1.exec()) {
        return {
            {"status", "error"},
            {"message", "查询失败：" + q1.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    if (!q1.next()) {
        return {
            {"status", "error"},
            {"message", "航班不存在"},
            {"data", QJsonValue()}
        };
    }

    int oldTotalSeats     = q1.value("total_seats").toInt();
    int oldRemainingSeats = q1.value("remaining_seats").toInt();

    // 剩余票数不能 > 总票数
    if (remainingSeats > totalSeats) {
        return {
            {"status", "error"},
            {"message", "remaining_seats 不能大于 total_seats"},
            {"data", QJsonValue()}
        };
    }

    // 不能把 total_seats 调小到低于已售出票数
    int sold = oldTotalSeats - oldRemainingSeats;
    if (totalSeats < sold) {
        return {
            {"status", "error"},
            {"message", "total_seats 不能小于已售出的票数"},
            {"data", QJsonValue()}
        };
    }

    // 更新航班
    QSqlQuery q2(DatabaseManager::instance().database());
    q2.prepare(R"(
        UPDATE Flight SET
            flight_number   = :flight_number,
            origin          = :origin,
            destination     = :destination,
            departure_time  = :departure_time,
            arrival_time    = :arrival_time,
            price           = :price,
            total_seats     = :total_seats,        -- FIX
            remaining_seats = :remaining_seats
        WHERE flight_id = :flight_id
    )");

    q2.bindValue(":flight_number",   flightNumber);
    q2.bindValue(":origin",          origin);
    q2.bindValue(":destination",     destination);
    q2.bindValue(":departure_time",  departureTime);
    q2.bindValue(":arrival_time",    arrivalTime);
    q2.bindValue(":price",           price);
    q2.bindValue(":total_seats",     totalSeats);        // FIX
    q2.bindValue(":remaining_seats", remainingSeats);
    q2.bindValue(":flight_id",       flightId);

    if (!q2.exec()) {
        return {
            {"status", "error"},
            {"message", "数据库更新失败：" + q2.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    return {
        {"status", "success"},
        {"message", "航班更新成功"},
        {"data", QJsonValue()}
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

    // 检查航班是否存在
    QSqlQuery q1(DatabaseManager::instance().database());
    q1.prepare(R"(
        SELECT flight_id, is_deleted
        FROM Flight
        WHERE flight_id = :flight_id
    )");
    q1.bindValue(":flight_id", flightId);

    if (!q1.exec()) {
        return {
            {"status", "error"},
            {"message", "查询失败：" + q1.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    if (!q1.next()) {
        return {
            {"status", "error"},
            {"message", "航班不存在"},
            {"data", QJsonValue()}
        };
    }

    // 已删除的航班不重复删除
    if (q1.value("is_deleted").toInt() == 1) {
        return {
            {"status", "error"},
            {"message", "航班已删除，无需重复操作"},
            {"data", QJsonValue()}
        };
    }

    // 软删除：设置 is_deleted = 1
    QSqlQuery q2(DatabaseManager::instance().database());
    q2.prepare(R"(
        UPDATE Flight
        SET is_deleted = 1
        WHERE flight_id = :flight_id
    )");
    q2.bindValue(":flight_id", flightId);

    if (!q2.exec()) {
        return {
            {"status", "error"},
            {"message", "删除航班失败：" + q2.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    return {
        {"status", "success"},
        {"message", "航班已成功删除"},
        {"data", QJsonValue()}
    };
}

// 管理员-获取所有用户
QJsonObject TcpServer::handleAdminGetAllUsers()
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(R"(
        SELECT
            user_id, username, is_admin, created_at
        FROM User
        ORDER BY user_id ASC
        LIMIT :limit
    )");

    query.bindValue(":limit", MAX_RETURN_ROWS);

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
        obj["is_admin"] = query.value("is_admin").toInt();
        obj["created_at"] = query.value("created_at").toString();

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
            b.booking_id,
            b.user_id,
            b.flight_id,
            b.status,
            b.booking_time,

            User.username,

            f.flight_number,
            f.model,
            f.origin,
            f.destination,
            f.departure_time,
            f.arrival_time,
            f.price,
            f.is_deleted
        FROM Booking b
        JOIN User   ON Booking.user_id  = User.user_id
        JOIN Flight f ON Booking.flight_id = Flight.flight_id
        ORDER BY Booking.booking_time DESC
        LIMIT :limit
    )");

    query.bindValue(":limit", MAX_RETURN_ROWS);

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

        obj["booking_id"]      = query.value("booking_id").toInt();
        obj["user_id"]         = query.value("user_id").toInt();
        obj["flight_id"]       = query.value("flight_id").toInt();
        obj["status"]          = query.value("status").toString();
        obj["booking_time"]    = query.value("booking_time").toString();

        obj["username"]        = query.value("username").toString();

        obj["flight_number"]   = query.value("flight_number").toString();
        obj["model"]           = query.value("model").toString();
        obj["origin"]          = query.value("origin").toString();
        obj["destination"]     = query.value("destination").toString();
        obj["departure_time"]  = query.value("departure_time").toString();
        obj["arrival_time"]    = query.value("arrival_time").toString();
        obj["price"]           = query.value("price").toDouble();
        obj["is_deleted"]      = query.value("is_deleted").toInt();

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

    if (!query.exec(R"(
        SELECT flight_id, flight_number, model, origin, destination,
               departure_time, arrival_time,
               total_seats, remaining_seats, price, is_deleted
        FROM Flight
        ORDER BY departure_time ASC
        LIMIT :limit
    )"))
    {
        return {
            {"status", "error"},
            {"message", "查询失败：" + query.lastError().text()},
            {"data", QJsonValue()}
        };
    }

    query.bindValue(":limit", MAX_RETURN_ROWS);
    QJsonArray arr;

    while (query.next()) {
        QJsonObject obj;

        obj["flight_id"]       = query.value("flight_id").toInt();
        obj["flight_number"]   = query.value("flight_number").toString();
        obj["model"]           = query.value("model").toString();
        obj["origin"]          = query.value("origin").toString();
        obj["destination"]     = query.value("destination").toString();
        obj["departure_time"]  = query.value("departure_time").toString();
        obj["arrival_time"]    = query.value("arrival_time").toString();
        obj["total_seats"]     = query.value("total_seats").toInt();
        obj["remaining_seats"] = query.value("remaining_seats").toInt();
        obj["price"]           = query.value("price").toDouble();
        obj["is_deleted"]      = query.value("is_deleted").toInt();

        arr.append(obj);
    }

    return {
        {"status", "success"},
        {"message", "查询成功"},
        {"data", arr}
    };
}


