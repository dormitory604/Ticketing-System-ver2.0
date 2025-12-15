#ifndef QML_BRIDGE_H
#define QML_BRIDGE_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QVariantList>
#include "network_manager.h"
#include "app_session.h"

class QmlBridge : public QObject
{
    Q_OBJECT

    // 会话信息
    Q_PROPERTY(QString currentUsername READ currentUsername NOTIFY currentUsernameChanged)
    Q_PROPERTY(int currentUserId READ currentUserId NOTIFY currentUserIdChanged)
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY isLoggedInChanged)
    Q_PROPERTY(bool isAdmin READ isAdmin NOTIFY isAdminChanged)

    // 搜索相关
    Q_PROPERTY(QVariantList searchResults READ searchResults NOTIFY searchResultsChanged)
    Q_PROPERTY(bool searchInProgress READ searchInProgress NOTIFY searchInProgressChanged)

    // 订单相关
    Q_PROPERTY(QVariantList myOrders READ myOrders NOTIFY myOrdersChanged)
    Q_PROPERTY(bool ordersInProgress READ ordersInProgress NOTIFY ordersInProgressChanged)

public:
    explicit QmlBridge(QObject *parent = nullptr);

    // 属性读取函数
    QString currentUsername() const { return AppSession::instance().username(); }
    int currentUserId() const { return AppSession::instance().userId(); }
    bool isLoggedIn() const { return AppSession::instance().userId() > 0; }
    bool isAdmin() const { return AppSession::instance().isAdmin(); }
    QVariantList searchResults() const { return m_searchResults; }
    QVariantList myOrders() const { return m_myOrders; }
    bool searchInProgress() const { return m_searchInProgress; }
    bool ordersInProgress() const { return m_ordersInProgress; }

public slots:
    // 登录/注册
    void login(const QString &username, const QString &password);
    void registerUser(const QString &username, const QString &password);
    void logout();

    // 搜索
    void searchFlights(const QString &origin, const QString &dest, const QString &date,
                       const QString &cabinClass = "", const QStringList &passengerTypes = {});

    // 预订
    void bookFlight(int flightId);

    // 订单
    void getMyOrders();
    void cancelOrder(int bookingId);

    // 个人资料
    void updateProfile(const QString &username, const QString &password);

    // 窗口管理
    void showRegisterWindow();
    void showSearchWindow();
    void showOrdersWindow();
    void showProfileWindow();
    void closeCurrentWindow();

signals:
    // 属性变更信号
    void currentUsernameChanged();
    void currentUserIdChanged();
    void isLoggedInChanged();
    void isAdminChanged();
    void searchResultsChanged();
    void myOrdersChanged();

    // 操作结果信号
    void loginSuccess(const QJsonObject &userData);
    void loginFailed(const QString &message);
    void registerSuccess(const QString &message);
    void registerFailed(const QString &message);
    void searchComplete();
    void bookingSuccess(const QJsonObject &bookingData);
    void bookingFailed(const QString &message);
    void ordersUpdated();
    void cancelOrderSuccess(const QString &message);
    void cancelOrderFailed(const QString &message);
    void searchInProgressChanged();
    void ordersInProgressChanged();
    void profileUpdateSuccess(const QString &message, const QJsonObject &userData);
    void profileUpdateFailed(const QString &message);
    void errorOccurred(const QString &message);

    // 窗口切换信号
    void requestShowRegister();
    void requestShowSearch();
    void requestShowOrders();
    void requestShowProfile();
    void requestCloseWindow();

private slots:
    void onLoginSuccess(const QJsonObject &userData);
    void onLoginFailed(const QString &message);
    void onRegisterSuccess(const QString &message);
    void onRegisterFailed(const QString &message);
    void onSearchResults(const QJsonArray &flights);
    void onBookingSuccess(const QJsonObject &bookingData);
    void onBookingFailed(const QString &message);
    void onMyOrdersResult(const QJsonArray &orders);
    void onMyOrdersFailed(const QString &message);
    void onCancelOrderSuccess(const QString &message);
    void onCancelOrderFailed(const QString &message);
    void onSearchFailed(const QString &message);
    void onProfileUpdateSuccess(const QString &message, const QJsonObject &userData);
    void onProfileUpdateFailed(const QString &message);
    void onGeneralError(const QString &message);
    void onUserChanged();

private:
    QVariantList m_searchResults;
    QVariantList m_myOrders;
    bool m_searchInProgress{false};
    bool m_ordersInProgress{false};
    QJsonObject m_pendingProfileUpdate;

    // 辅助函数：将 QJsonArray 转换为 QVariantList
    static QVariantList jsonArrayToVariantList(const QJsonArray &jsonArray);
};

#endif // QML_BRIDGE_H
