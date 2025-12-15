#include "qml_bridge.h"
#include <QDebug>
#include <QVariant>
#include <QDate>

QmlBridge::QmlBridge(QObject *parent)
    : QObject(parent)
{
    NetworkManager &nm = NetworkManager::instance();

    // 连接所有 NetworkManager 信号
    connect(&nm, &NetworkManager::loginSuccess, this, &QmlBridge::onLoginSuccess);
    connect(&nm, &NetworkManager::loginFailed, this, &QmlBridge::onLoginFailed);
    connect(&nm, &NetworkManager::registerSuccess, this, &QmlBridge::onRegisterSuccess);
    connect(&nm, &NetworkManager::registerFailed, this, &QmlBridge::onRegisterFailed);
    connect(&nm, &NetworkManager::searchResults, this, &QmlBridge::onSearchResults);
    connect(&nm, &NetworkManager::searchFailed, this, &QmlBridge::onSearchFailed);
    connect(&nm, &NetworkManager::bookingSuccess, this, &QmlBridge::onBookingSuccess);
    connect(&nm, &NetworkManager::bookingFailed, this, &QmlBridge::onBookingFailed);
    connect(&nm, &NetworkManager::myOrdersResult, this, &QmlBridge::onMyOrdersResult);
    connect(&nm, &NetworkManager::myOrdersFailed, this, &QmlBridge::onMyOrdersFailed);
    connect(&nm, &NetworkManager::cancelOrderSuccess, this, &QmlBridge::onCancelOrderSuccess);
    connect(&nm, &NetworkManager::cancelOrderFailed, this, &QmlBridge::onCancelOrderFailed);
    connect(&nm, &NetworkManager::profileUpdateSuccess, this, &QmlBridge::onProfileUpdateSuccess);
    connect(&nm, &NetworkManager::profileUpdateFailed, this, &QmlBridge::onProfileUpdateFailed);
    connect(&nm, &NetworkManager::generalError, this, &QmlBridge::onGeneralError);

    // 连接 AppSession 信号
    connect(&AppSession::instance(), &AppSession::userChanged, this, &QmlBridge::onUserChanged);
}

void QmlBridge::login(const QString &username, const QString &password)
{
    NetworkManager::instance().sendLoginRequest(username, password);
}

void QmlBridge::registerUser(const QString &username, const QString &password)
{
    NetworkManager::instance().sendRegisterRequest(username, password);
}

void QmlBridge::logout()
{
    AppSession::instance().clear();
    emit isLoggedInChanged();
    emit currentUsernameChanged();
    emit currentUserIdChanged();
    emit isAdminChanged();
}

void QmlBridge::searchFlights(const QString &origin, const QString &dest, const QString &date,
                              const QString &cabinClass, const QStringList &passengerTypes)
{
    const QString trimmedOrigin = origin.trimmed();
    const QString trimmedDest = dest.trimmed();

    if (!trimmedOrigin.isEmpty() && trimmedOrigin.compare(trimmedDest, Qt::CaseInsensitive) == 0)
    {
        emit errorOccurred(QStringLiteral("出发地和目的地不能相同"));
        return;
    }

    QString normalizedDate = date.trimmed();
    if (normalizedDate.isEmpty())
    {
        normalizedDate = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
    }

    if (m_searchInProgress)
    {
        emit errorOccurred(QStringLiteral("上一条查询尚未完成"));
        return;
    }

    m_searchInProgress = true;
    emit searchInProgressChanged();
    NetworkManager::instance().sendSearchRequest(trimmedOrigin, trimmedDest, normalizedDate, cabinClass, passengerTypes);
}

void QmlBridge::bookFlight(int flightId)
{
    NetworkManager::instance().bookFlightRequest(AppSession::instance().userId(), flightId);
}

void QmlBridge::getMyOrders()
{
    if (m_ordersInProgress)
    {
        emit errorOccurred(QStringLiteral("订单请求正在处理中"));
        return;
    }

    const int userId = AppSession::instance().userId();
    if (userId <= 0)
    {
        emit errorOccurred(QStringLiteral("请先登录"));
        return;
    }

    m_ordersInProgress = true;
    emit ordersInProgressChanged();
    NetworkManager::instance().getMyOrdersRequest(userId);
}

void QmlBridge::cancelOrder(int bookingId)
{
    NetworkManager::instance().cancelOrderRequest(bookingId);
}

void QmlBridge::updateProfile(const QString &username, const QString &password)
{
    const int userId = AppSession::instance().userId();
    if (userId <= 0)
    {
        emit errorOccurred(QStringLiteral("请先登录"));
        return;
    }
    QJsonObject pending;
    pending["user_id"] = userId;
    pending["username"] = username;
    m_pendingProfileUpdate = pending;
    NetworkManager::instance().updateProfileRequest(userId, username, password);
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

void QmlBridge::showProfileWindow()
{
    emit requestShowProfile();
}

void QmlBridge::closeCurrentWindow()
{
    emit requestCloseWindow();
}

// 信号转发槽函数
void QmlBridge::onLoginSuccess(const QJsonObject &userData)
{
    AppSession::instance().setCurrentUser(userData);
    emit loginSuccess(userData);
    emit isLoggedInChanged();
    emit currentUsernameChanged();
    emit currentUserIdChanged();
    emit isAdminChanged();
}

void QmlBridge::onLoginFailed(const QString &message)
{
    emit loginFailed(message);
}

void QmlBridge::onRegisterSuccess(const QString &message)
{
    emit registerSuccess(message);
}

void QmlBridge::onRegisterFailed(const QString &message)
{
    emit registerFailed(message);
}

QVariantList QmlBridge::jsonArrayToVariantList(const QJsonArray &jsonArray)
{
    QVariantList result;
    for (const QJsonValue &value : jsonArray)
    {
        if (value.isObject())
        {
            result.append(value.toObject().toVariantMap());
        }
        else
        {
            result.append(value.toVariant());
        }
    }
    return result;
}

void QmlBridge::onSearchResults(const QJsonArray &flights)
{
    m_searchResults = jsonArrayToVariantList(flights);
    emit searchResultsChanged();
    emit searchComplete();
    if (m_searchInProgress)
    {
        m_searchInProgress = false;
        emit searchInProgressChanged();
    }
}

void QmlBridge::onSearchFailed(const QString &message)
{
    if (m_searchInProgress)
    {
        m_searchInProgress = false;
        emit searchInProgressChanged();
    }
    emit errorOccurred(message);
}

void QmlBridge::onBookingSuccess(const QJsonObject &bookingData)
{
    emit bookingSuccess(bookingData);
}

void QmlBridge::onBookingFailed(const QString &message)
{
    emit bookingFailed(message);
}

void QmlBridge::onMyOrdersResult(const QJsonArray &orders)
{
    m_myOrders = jsonArrayToVariantList(orders);
    emit myOrdersChanged();
    emit ordersUpdated();
    if (m_ordersInProgress)
    {
        m_ordersInProgress = false;
        emit ordersInProgressChanged();
    }
}

void QmlBridge::onMyOrdersFailed(const QString &message)
{
    if (m_ordersInProgress)
    {
        m_ordersInProgress = false;
        emit ordersInProgressChanged();
    }
    emit errorOccurred(message);
}

void QmlBridge::onCancelOrderSuccess(const QString &message)
{
    emit cancelOrderSuccess(message);
    // 自动刷新订单列表
    getMyOrders();
}

void QmlBridge::onCancelOrderFailed(const QString &message)
{
    emit cancelOrderFailed(message);
}

void QmlBridge::onProfileUpdateSuccess(const QString &message, const QJsonObject &userData)
{
    QJsonObject updated = userData;
    if (updated.isEmpty())
    {
        updated = AppSession::instance().currentUser();
        if (updated.isEmpty())
        {
            updated["user_id"] = AppSession::instance().userId();
            updated["is_admin"] = AppSession::instance().isAdmin() ? 1 : 0;
        }
        if (!m_pendingProfileUpdate.isEmpty())
        {
            const QString username = m_pendingProfileUpdate.value("username").toString();
            if (!username.isEmpty())
            {
                updated["username"] = username;
            }
        }
    }
    AppSession::instance().setCurrentUser(updated);
    m_pendingProfileUpdate = QJsonObject();
    emit profileUpdateSuccess(message, updated);
    emit currentUsernameChanged();
}

void QmlBridge::onProfileUpdateFailed(const QString &message)
{
    m_pendingProfileUpdate = QJsonObject();
    emit profileUpdateFailed(message);
}

void QmlBridge::onGeneralError(const QString &message)
{
    emit errorOccurred(message);
}

void QmlBridge::onUserChanged()
{
    emit isLoggedInChanged();
    emit currentUsernameChanged();
    emit currentUserIdChanged();
    emit isAdminChanged();
}
