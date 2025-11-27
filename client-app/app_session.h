#ifndef APP_SESSION_H
#define APP_SESSION_H

#include <QObject>
#include <QJsonObject>

class AppSession : public QObject
{
    Q_OBJECT
public:
    static AppSession& instance();

    void setCurrentUser(const QJsonObject& user);
    QJsonObject currentUser() const;
    int userId() const;
    QString username() const;
    bool isAdmin() const;
    bool isLoggedIn() const;
    void clear();

signals:
    void userChanged(const QJsonObject& user);

private:
    explicit AppSession(QObject *parent = nullptr);
    ~AppSession() = default;
    AppSession(const AppSession&) = delete;
    AppSession& operator=(const AppSession&) = delete;

    QJsonObject m_currentUser;
};

#endif // APP_SESSION_H
