#include "qml_bridge.h"
#include <QDebug>
#include <QVariant>

QmlBridge::QmlBridge(QObject *parent)
    : QObject(parent)
{
    NetworkManager& nm = NetworkManager::instance();
    
    // 连接所有 NetworkManager 信号
    connect(&nm, &NetworkManager::loginSuccess, this, &QmlBridge::onLoginSuccess);
    connect(&nm, &NetworkManager::loginFailed, this, &QmlBridge::onLoginFailed);
    connect(&nm, &NetworkManager::registerSuccess, this, &QmlBridge::onRegisterSuccess);
    connect(&nm, &NetworkManager::registerFailed, this, &QmlBridge::onRegisterFailed);
    connect(&nm, &NetworkManager::searchResults, this, &QmlBridge::onSearchResults);
    connect(&nm, &NetworkManager::bookingSuccess, this, &QmlBridge::onBookingSuccess);
    connect(&nm, &NetworkManager::bookingFailed, this, &QmlBridge::onBookingFailed);
    connect(&nm, &NetworkManager::myOrdersResult, this, &QmlBridge::onMyOrdersResult);
    connect(&nm, &NetworkManager::cancelOrderSuccess, this, &QmlBridge::onCancelOrderSuccess);
    connect(&nm, &NetworkManager::cancelOrderFailed, this, &QmlBridge::onCancelOrderFailed);
    connect(&nm, &NetworkManager::myFavoritesResult, this, &QmlBridge::onMyFavoritesResult);
    connect(&nm, &NetworkManager::addFavoriteSuccess, this, &QmlBridge::onAddFavoriteSuccess);
    connect(&nm, &NetworkManager::addFavoriteFailed, this, &QmlBridge::onAddFavoriteFailed);
    connect(&nm, &NetworkManager::removeFavoriteSuccess, this, &QmlBridge::onRemoveFavoriteSuccess);
    connect(&nm, &NetworkManager::removeFavoriteFailed, this, &QmlBridge::onRemoveFavoriteFailed);
    connect(&nm, &NetworkManager::profileUpdateSuccess, this, &QmlBridge::onProfileUpdateSuccess);
    connect(&nm, &NetworkManager::profileUpdateFailed, this, &QmlBridge::onProfileUpdateFailed);
    connect(&nm, &NetworkManager::generalError, this, &QmlBridge::onGeneralError);
    
    // 连接 AppSession 信号
    connect(&AppSession::instance(), &AppSession::userChanged, this, &QmlBridge::onUserChanged);
}

void QmlBridge::login(const QString& username, const QString& password)
{
    NetworkManager::instance().sendLoginRequest(username, password);
}

void QmlBridge::registerUser(const QString& username, const QString& password)
{
    NetworkManager::instance().sendRegisterRequest(username, password);
}

void QmlBridge::logout()
{
    AppSession::instance().clear();
    emit isLoggedInChanged();
    emit currentUsernameChanged();
    emit currentUserIdChanged();
}

void QmlBridge::searchFlights(const QString& origin, const QString& dest, const QString& date,
                               const QString& cabinClass, const QStringList& passengerTypes)
{
    NetworkManager::instance().sendSearchRequest(origin, dest, date, cabinClass, passengerTypes);
}

void QmlBridge::bookFlight(int flightId)
{
    NetworkManager::instance().bookFlightRequest(AppSession::instance().userId(), flightId);
}

void QmlBridge::getMyOrders()
{
    NetworkManager::instance().getMyOrdersRequest(AppSession::instance().userId());
}

void QmlBridge::cancelOrder(int bookingId)
{
    NetworkManager::instance().cancelOrderRequest(bookingId);
}

void QmlBridge::getMyFavorites()
{
    NetworkManager::instance().getMyFavoritesRequest(AppSession::instance().userId());
}

void QmlBridge::addFavorite(int flightId)
{
    NetworkManager::instance().addFavoriteRequest(AppSession::instance().userId(), flightId);
}

void QmlBridge::removeFavorite(int flightId)
{
    NetworkManager::instance().removeFavoriteRequest(AppSession::instance().userId(), flightId);
}

void QmlBridge::updateProfile(const QString& username, const QString& password)
{
    NetworkManager::instance().updateProfileRequest(AppSession::instance().userId(), username, password);
}

void QmlBridge::showRegisterWindow()
{
    emit requestShowRegister();
}

void QmlBridge::showSearchWindow()
{
    emit requestShowSearch();
}

void QmlBridge::showOrdersWindow()
{
    emit requestShowOrders();
}

void QmlBridge::showFavoritesWindow()
{
    emit requestShowFavorites();
}

void QmlBridge::showProfileWindow()
{
    emit requestShowProfile();
}

void QmlBridge::closeCurrentWindow()
{
    emit requestCloseWindow();
}

// 信号转发槽函数
void QmlBridge::onLoginSuccess(const QJsonObject& userData)
{
    AppSession::instance().setCurrentUser(userData);
    emit loginSuccess(userData);
    emit isLoggedInChanged();
    emit currentUsernameChanged();
    emit currentUserIdChanged();
}

void QmlBridge::onLoginFailed(const QString& message)
{
    emit loginFailed(message);
}

void QmlBridge::onRegisterSuccess(const QString& message)
{
    emit registerSuccess(message);
}

void QmlBridge::onRegisterFailed(const QString& message)
{
    emit registerFailed(message);
}

QVariantList QmlBridge::jsonArrayToVariantList(const QJsonArray& jsonArray)
{
    QVariantList result;
    for (const QJsonValue& value : jsonArray) {
        if (value.isObject()) {
            result.append(value.toObject().toVariantMap());
        } else {
            result.append(value.toVariant());
        }
    }
    return result;
}

void QmlBridge::onSearchResults(const QJsonArray& flights)
{
    m_searchResults = jsonArrayToVariantList(flights);
    emit searchResultsChanged();
    emit searchComplete();
}

void QmlBridge::onBookingSuccess(const QJsonObject& bookingData)
{
    emit bookingSuccess(bookingData);
}

void QmlBridge::onBookingFailed(const QString& message)
{
    emit bookingFailed(message);
}

void QmlBridge::onMyOrdersResult(const QJsonArray& orders)
{
    m_myOrders = jsonArrayToVariantList(orders);
    emit myOrdersChanged();
    emit ordersUpdated();
}

void QmlBridge::onCancelOrderSuccess(const QString& message)
{
    emit cancelOrderSuccess(message);
    // 自动刷新订单列表
    getMyOrders();
}

void QmlBridge::onCancelOrderFailed(const QString& message)
{
    emit cancelOrderFailed(message);
}

void QmlBridge::onMyFavoritesResult(const QJsonArray& favorites)
{
    m_myFavorites = jsonArrayToVariantList(favorites);
    emit myFavoritesChanged();
    emit favoritesUpdated();
}

void QmlBridge::onAddFavoriteSuccess(const QString& message)
{
    emit addFavoriteSuccess(message);
}

void QmlBridge::onAddFavoriteFailed(const QString& message)
{
    emit addFavoriteFailed(message);
}

void QmlBridge::onRemoveFavoriteSuccess(const QString& message)
{
    emit removeFavoriteSuccess(message);
    // 自动刷新收藏列表
    getMyFavorites();
}

void QmlBridge::onRemoveFavoriteFailed(const QString& message)
{
    emit removeFavoriteFailed(message);
}

void QmlBridge::onProfileUpdateSuccess(const QString& message, const QJsonObject& userData)
{
    AppSession::instance().setCurrentUser(userData);
    emit profileUpdateSuccess(message, userData);
    emit currentUsernameChanged();
}

void QmlBridge::onProfileUpdateFailed(const QString& message)
{
    emit profileUpdateFailed(message);
}

void QmlBridge::onGeneralError(const QString& message)
{
    emit errorOccurred(message);
}

void QmlBridge::onUserChanged()
{
    emit isLoggedInChanged();
    emit currentUsernameChanged();
    emit currentUserIdChanged();
}


