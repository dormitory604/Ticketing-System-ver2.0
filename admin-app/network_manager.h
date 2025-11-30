#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    // 保证程序里面只有一个networkmanager实例
    static NetworkManager& instance()
    {
        static NetworkManager instance;
        return instance;
    }

    // 连接控制
    void connectToServer();  // 连接服务器端口：127.0.0.1:12345
    bool isConnected() const;  // 查看是否与服务器连接

    // 发送接口(C2S)
    // 登录(对应server-app中的handleLogin)
    void sendAdminLoginRequest(const QString& username, const QString& password);

    // 航班查询(对应server-app中的handleSearchFlights)
    void sendGetAllFlightsRequest();

    // 项目接口文档3.3中的5个管理员接口
    // 添加航班(对应server-app与管理员接口handleAdminAddFlight)
    void sendAdminAddFlightRequest(const QJsonObject& flightData);

    // 修改航班(对应server-app与管理员接口handleAdminUpdateFlight)
    void sendAdminUpdateFlightRequest(int flightId, const QJsonObject& changes);

    // 删除航班(对应server-app与管理员接口handleAdminDeleteFlight)
    void sendAdminDeleteFlightRequest(int flightId);

    // 获取所有用户(对应server-app与管理员接口handleAdminGetAllUsers)
    void sendAdminGetAllUsersRequest();

    // 获取所有订单(对应server-app与管理员接口handleAdminGetAllBookings)
    void sendAdminGetAllBookingsRequest();


signals:
    // --- 接收信号 (S2C) ---

    // 登录结果: success判断登录是否成功, isAdmin判断登录的账号是不是管理员, message打印登陆失败的信息
    void loginResult(bool success, bool isAdmin, const QString& message);

    // 收到航班列表(用于刷新航班管理表格)
    void allFlightsReceived(const QJsonArray& flights);

    // 收到用户列表(用于刷新用户管理表格)
    void allUsersReceived(const QJsonArray& users);

    // 收到订单列表(用于刷新订单管理表格)
    void allBookingsReceived(const QJsonArray& bookings);

    // 通用操作成功/失败信号 (用于 添加/删除/修改 后的弹窗提示)
    void adminOperationSuccess(const QString& message);
    void adminOperationFailed(const QString& message);

private:
    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();

    QTcpSocket* m_socket;
    const QString SERVER_IP = "127.0.0.1";
    const quint16 SERVER_PORT = 12345; // 对应 Server main.cpp 里的端口

    // 辅助发送函数
    void send(const QJsonObject& json);

private slots:
    void onReadyRead();      // 处理服务器回信
    void onConnected();
    void onDisconnected();
};

#endif // NETWORK_MANAGER_H
