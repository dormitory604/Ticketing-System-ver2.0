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
        // (这里也可以发送一个错误JSON回去)
        return; // TO DO
    }

    QJsonObject request = jsonDoc.object();
    qDebug() << "解析JSON请求:" << request;

    QJsonObject response = handleRequest(request);

    sendJsonResponse(socket, response);
}

// 当客户端断开连接时
void TcpServer::onDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    qInfo() << "客户端断开连接:" << socket->peerAddress().toString();
    socket->deleteLater(); // 这里不能马上删除
}

// 用来发送JSON响应的辅助函数，此处返回的是QByteArray
void TcpServer::sendJsonResponse(QTcpSocket* socket, const QJsonObject& response)
{
    QJsonDocument doc(response);
    QByteArray data = doc.toJson();
    socket->write(data);
    qDebug() << "发送JSON响应:" << response;
}

/// 以下为服务器具体需求功能实现

// 请求分发路由器
QJsonObject TcpServer::handleRequest(const QJsonObject& request)
{
    QString action = request["action"].toString();
    QJsonObject data = request["data"].toObject();

    if (action == "login") {
        return handleLogin(data);
    }
    if (action == "search_flights") {
        return handleSearchFlights(data);
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

// 处理注册（TODO）


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

// 预定航班（TODO）
