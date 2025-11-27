#include "admin_login_window.h"
#include "ui_admin_login_window.h"
#include "network_manager.h"
#include <QMessageBox>

AdminLoginWindow::AdminLoginWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AdminLoginWindow)
{
    ui->setupUi(this);

    // 连接networkmanager
    connect(&NetworkManager::instance(), &NetworkManager::loginResult, this, &AdminLoginWindow::handleLoginResult);
}

AdminLoginWindow::~AdminLoginWindow()
{
    delete ui;
}

// 点击“登录”按钮的槽函数
void AdminLoginWindow::on_loginButton_clicked()
{
    QString username = ui->usernameEdit->text();
    QString password = ui->passwordEdit->text();

    // 简单校验一下不能为空
    if(username.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(this, "警告", "账号或密码不能为空！");
        return;
    }

    // 禁用按钮，防止用户狂点
    ui->loginButton->setEnabled(false);
    ui->loginButton->setText("登录中...");

    // 发送请求给服务器
    NetworkManager::instance().sendAdminLoginRequest(username, password);
}

// 接收NetworkManager发来的登录结果
void AdminLoginWindow::handleLoginResult(bool success, bool is_admin, const QString& message)
{
    // 先恢复按钮状态
    ui->loginButton->setEnabled(true);
    ui->loginButton->setText("登录");

    // 登录成功，判断是不是管理员
    if(success)
    {
        // 身份是管理员，跳转到管理员页面
        if(is_admin)
        {
            this->accept();
        }
        // 身份不是管理员，拒绝进入管理员界面
        else
        {
            QMessageBox::warning(this, "权限不足", "您不是管理员，无法登录后台！");
        }
    }
    // 登录失败，弹出信息，账号或密码错误
    else
    {
        QMessageBox::warning(this, "登录失败", message);
    }
}
