#ifndef QML_BRIDGE_H
#define QML_BRIDGE_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include "network_manager.h"
#include "app_session.h"

class QmlBridge : public QObject
{
    Q_OBJECT
    
    // 会话信息
    Q_PROPERTY(QString currentUsername READ currentUsername NOTIFY currentUsernameChanged)
    Q_PROPERTY(int currentUserId READ currentUserId NOTIFY currentUserIdChanged)
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY isLoggedInChanged)
    
    // 搜索相关
    Q_PROPERTY(QJsonArray searchResults READ searchResults NOTIFY searchResultsChanged)
    
    // 订单相关
    Q_PROPERTY(QJsonArray myOrders READ myOrders NOTIFY myOrdersChanged)
    
    // 收藏相关
    Q_PROPERTY(QJsonArray myFavorites READ myFavorites NOTIFY myFavoritesChanged)

public:
    explicit QmlBridge(QObject *parent = nullptr);
    
    // 属性读取函数
    QString currentUsername() const { return AppSession::instance().username(); }
    int currentUserId() const { return AppSession::instance().userId(); }
    bool isLoggedIn() const { return AppSession::instance().userId() > 0; }
    QJsonArray searchResults() const { return m_searchResults; }
    QJsonArray myOrders() const { return m_myOrders; }
    QJsonArray myFavorites() const { return m_myFavorites; }

public slots:
    // 登录/注册
    void login(const QString& username, const QString& password);
    void registerUser(const QString& username, const QString& password);
    void logout();
    
    // 搜索
    void searchFlights(const QString& origin, const QString& dest, const QString& date,
                       const QString& cabinClass = "", const QStringList& passengerTypes = {});
    
    // 预订
    void bookFlight(int flightId);
    
    // 订单
    void getMyOrders();
    void cancelOrder(int bookingId);
    
    // 收藏
    void getMyFavorites();
    void addFavorite(int flightId);
    void removeFavorite(int flightId);
    
    // 个人资料
    void updateProfile(const QString& username, const QString& password);
    
    // 窗口管理
    void showRegisterWindow();
    void showSearchWindow();
    void showOrdersWindow();
    void showFavoritesWindow();
    void showProfileWindow();
    void closeCurrentWindow();

signals:
    // 属性变更信号
    void currentUsernameChanged();
    void currentUserIdChanged();
    void isLoggedInChanged();
    void searchResultsChanged();
    void myOrdersChanged();
    void myFavoritesChanged();
    
    // 操作结果信号
    void loginSuccess(const QJsonObject& userData);
    void loginFailed(const QString& message);
    void registerSuccess(const QString& message);
    void registerFailed(const QString& message);
    void searchComplete();
    void bookingSuccess(const QJsonObject& bookingData);
    void bookingFailed(const QString& message);
    void ordersUpdated();
    void cancelOrderSuccess(const QString& message);
    void cancelOrderFailed(const QString& message);
    void favoritesUpdated();
    void addFavoriteSuccess(const QString& message);
    void addFavoriteFailed(const QString& message);
    void removeFavoriteSuccess(const QString& message);
    void removeFavoriteFailed(const QString& message);
    void profileUpdateSuccess(const QString& message, const QJsonObject& userData);
    void profileUpdateFailed(const QString& message);
    void errorOccurred(const QString& message);
    
    // 窗口切换信号
    void requestShowRegister();
    void requestShowSearch();
    void requestShowOrders();
    void requestShowFavorites();
    void requestShowProfile();
    void requestCloseWindow();

private slots:
    void onLoginSuccess(const QJsonObject& userData);
    void onLoginFailed(const QString& message);
    void onRegisterSuccess(const QString& message);
    void onRegisterFailed(const QString& message);
    void onSearchResults(const QJsonArray& flights);
    void onBookingSuccess(const QJsonObject& bookingData);
    void onBookingFailed(const QString& message);
    void onMyOrdersResult(const QJsonArray& orders);
    void onCancelOrderSuccess(const QString& message);
    void onCancelOrderFailed(const QString& message);
    void onMyFavoritesResult(const QJsonArray& favorites);
    void onAddFavoriteSuccess(const QString& message);
    void onAddFavoriteFailed(const QString& message);
    void onRemoveFavoriteSuccess(const QString& message);
    void onRemoveFavoriteFailed(const QString& message);
    void onProfileUpdateSuccess(const QString& message, const QJsonObject& userData);
    void onProfileUpdateFailed(const QString& message);
    void onGeneralError(const QString& message);
    void onUserChanged();

private:
    QJsonArray m_searchResults;
    QJsonArray m_myOrders;
    QJsonArray m_myFavorites;
};

#endif // QML_BRIDGE_H

