#include "app_session.h"

AppSession &AppSession::instance()
{
    static AppSession instance;
    return instance;
}

AppSession::AppSession(QObject *parent)
    : QObject(parent)
{
}

void AppSession::setCurrentUser(const QJsonObject &user)
{
    if (m_currentUser != user) {
        m_currentUser = user;
        emit userChanged(m_currentUser);
    }
}

QJsonObject AppSession::currentUser() const
{
    return m_currentUser;
}

int AppSession::userId() const
{
    return m_currentUser.value("user_id").toInt();
}

QString AppSession::username() const
{
    return m_currentUser.value("username").toString();
}

bool AppSession::isAdmin() const
{
    return m_currentUser.value("is_admin").toInt() == 1;
}

bool AppSession::isLoggedIn() const
{
    return !m_currentUser.isEmpty();
}

void AppSession::clear()
{
    if (!m_currentUser.isEmpty()) {
        m_currentUser = QJsonObject();
        emit userChanged(m_currentUser);
    }
}
