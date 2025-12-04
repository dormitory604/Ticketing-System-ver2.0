#ifndef ADMIN_LOGIN_WINDOW_H
#define ADMIN_LOGIN_WINDOW_H

#include <QDialog>

namespace Ui {
class AdminLoginWindow;
}

class AdminLoginWindow : public QDialog
{
    Q_OBJECT

public:
    explicit AdminLoginWindow(QWidget *parent = nullptr);
    ~AdminLoginWindow();

private slots:
    void on_loginButton_clicked();  // 点击“登录”按钮的槽函数
    void handleLoginResult(bool success, bool isAdmin, const QString& message);  // 接收NetworkManager发来的登录结果

private:
    Ui::AdminLoginWindow *ui;
};

#endif // ADMIN_LOGIN_WINDOW_H
