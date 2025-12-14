/*
这里的实现和server-app中的单例tcp_server.h高度相似，
用于在客户端实现统一的信息收发，和服务器端进行交流。
*/
#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QQueue>
#include <QByteArray>
#include <QStringList>

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    static NetworkManager &instance()
    {
        static NetworkManager instance;
        return instance;
    }

    void connectToServer(const QString &host, quint16 port);

    // Tag注册功能
    void sendTagRegistration(const QString &tag);
    bool isTagRegistered() const;
    QString generateUniqueTag() const;

    // 定义所有"发送"函数
    // 这是给UI界面调用的 (例如: on_loginButton_clicked)
    void sendLoginRequest(const QString &username, const QString &password);
    void sendSearchRequest(const QString &origin, const QString &dest, const QString &date,
                           const QString &cabinClass = QString(),
                           const QStringList &passengerTypes = {});
    void sendRegisterRequest(const QString &username, const QString &password);
    void bookFlightRequest(int userId, int flightId);
    void getMyOrdersRequest(int userId);
    void cancelOrderRequest(int bookingId);
    void updateProfileRequest(int userId, const QString &username, const QString &password);
    // ... (注意，每个action都对应一个发送函数，如果后续要新增这里也要加)

signals:
    // 定义所有"接收"信号
    // 这是发给UI界面的 (例如: onLoginSuccess)
    void connected();
    void disconnected();
    void tagRegistered();
    void tagRegistrationFailed(const QString &message);
    void loginSuccess(const QJsonObject &userData);
    void loginFailed(const QString &message);
    void searchResults(const QJsonArray &flights);
    void searchFailed(const QString &message);
    void registerSuccess(const QString &message);
    void registerFailed(const QString &message);
    void bookingSuccess(const QJsonObject &bookingData);
    void bookingFailed(const QString &message);
    void myOrdersResult(const QJsonArray &orders);
    void myOrdersFailed(const QString &message);
    void cancelOrderSuccess(const QString &message);
    void cancelOrderFailed(const QString &message);
    void profileUpdateSuccess(const QString &message, const QJsonObject &userData);
    void profileUpdateFailed(const QString &message);
    // ... (如果后续要加加在这里)
    void generalError(const QString &message);

private slots:
    void onConnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);
    void onDisconnected();

private:
    explicit NetworkManager(QObject *parent = nullptr); // 和server-app中类似私有构造，防止出现多个实例
    ~NetworkManager();
    // 禁用拷贝
    NetworkManager(const NetworkManager &) = delete;
    NetworkManager &operator=(const NetworkManager &) = delete;

    struct PendingRequest
    {
        QString action;
        QJsonObject payload;
    };

    QTcpSocket *m_socket;
    bool m_tagRegistered;
    QString m_clientTag;
    QQueue<PendingRequest> m_pendingRequests;
    QByteArray m_receiveBuffer;
    bool m_reconnectPending;
    QString m_lastHost;
    quint16 m_lastPort;

    void sendJsonRequest(const QJsonObject &request);
    void writeFramedJson(const QJsonDocument &document);
    void emitActionFailed(const QString &action, const QString &message);
    QJsonObject detachPendingPayload(QString &action);
    void processLengthPrefixedBuffer();
    void handleResponseObject(const QJsonObject &response);
    void reconnectToLastEndpoint();
};

#endif // NETWORKMANAGER_H
