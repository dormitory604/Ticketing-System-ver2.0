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

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    static NetworkManager& instance() {
        static NetworkManager instance;
        return instance;
    }

    void connectToServer(const QString& host, quint16 port);

    // Tag注册功能
    void sendTagRegistration(const QString& tag);
    bool isTagRegistered() const;
    QString generateUniqueTag() const;

    // 定义所有"发送"函数
    // 这是给UI界面调用的 (例如: on_loginButton_clicked)
    void sendLoginRequest(const QString& username, const QString& password);
    void sendSearchRequest(const QString& origin, const QString& dest, const QString& date,
                           const QString& cabinClass = QString(),
                           const QStringList& passengerTypes = {});
    void sendRegisterRequest(const QString& username, const QString& password);
    void bookFlightRequest(int userId, int flightId);
    void getMyOrdersRequest(int userId);
    void cancelOrderRequest(int bookingId);
    void updateProfileRequest(int userId, const QString& username, const QString& password);
    void addFavoriteRequest(int userId, int flightId);
    void removeFavoriteRequest(int userId, int flightId);
    void getMyFavoritesRequest(int userId);
    // ... (注意，每个action都对应一个发送函数，如果后续要新增这里也要加)

signals:
    // 定义所有"接收"信号
    // 这是发给UI界面的 (例如: onLoginSuccess)
    void connected();
    void disconnected();
    void tagRegistered();
    void tagRegistrationFailed(const QString& message);
    void loginSuccess(const QJsonObject& userData);
    void loginFailed(const QString& message);
    void searchResults(const QJsonArray& flights);
    void registerSuccess(const QString& message);
    void registerFailed(const QString& message);
    void bookingSuccess(const QJsonObject& bookingData);
    void bookingFailed(const QString& message);
    void myOrdersResult(const QJsonArray& orders);
    void cancelOrderSuccess(const QString& message);
    void cancelOrderFailed(const QString& message);
    void profileUpdateSuccess(const QString& message, const QJsonObject& userData);
    void profileUpdateFailed(const QString& message);
    void addFavoriteSuccess(const QString& message);
    void addFavoriteFailed(const QString& message);
    void removeFavoriteSuccess(const QString& message);
    void removeFavoriteFailed(const QString& message);
    void myFavoritesResult(const QJsonArray& favorites);
    // ... (如果后续要加加在这里)
    void generalError(const QString& message);


private slots:
    void onConnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);
    void onDisconnected();

private:
    explicit NetworkManager(QObject *parent = nullptr); // 和server-app中类似私有构造，防止出现多个实例
    ~NetworkManager();
    // 禁用拷贝
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    QTcpSocket *m_socket;
    bool m_tagRegistered;
    QString m_clientTag;
    QQueue<QString> m_pendingActions;

    void sendJsonRequest(const QJsonObject& request);

#ifdef USE_FAKE_SERVER
    void emitFakeLoginResponse(const QString& username);
    void emitFakeSearchResults(const QString& origin, const QString& dest, const QString& date);
    void emitFakeRegisterResponse(const QString& username);
    void emitFakeBookingResponse(int userId, int flightId);
    void emitFakeOrdersResponse(int userId);
    void emitFakeCancelResponse(int bookingId);
    void emitFakeProfileUpdateResponse(int userId, const QString& username);
    void emitFakeAddFavoriteResponse(int userId, int flightId);
    void emitFakeRemoveFavoriteResponse(int userId, int flightId);
    void emitFakeFavoritesResponse(int userId);
#endif
};

#endif // NETWORKMANAGER_H
