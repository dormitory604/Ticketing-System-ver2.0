/*构建登陆窗口*/
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "network_manager.h"
#include <QMessageBox>
#include "search_window.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 绑定 NetworkManager 的信号到本窗口的槽函数
    connect(&NetworkManager::instance(), &NetworkManager::loginSuccess,
            this, &MainWindow::onLoginSuccess);
    connect(&NetworkManager::instance(), &NetworkManager::loginFailed,
            this, &MainWindow::onLoginFailed);
    connect(&NetworkManager::instance(), &NetworkManager::generalError,
            this, &MainWindow::onLoginFailed);
}

MainWindow::~MainWindow()
{
    delete ui;
}


// 当用户点击“登录”按钮时
void MainWindow::on_loginButton_clicked()
{
    QString user = ui->usernameLineEdit->text();
    QString pass = ui->passwordLineEdit->text();

    // 调用 NetworkManager 发送请求
    NetworkManager::instance().sendLoginRequest(user, pass);

    // (这里可以加一个 loading 动画之类的（但是似乎也不会loading太久）)
}

// 登录成功的slot函数
void MainWindow::onLoginSuccess(const QJsonObject& userData)
{
    // (userData 里有 user_id, is_admin 等信息, 可以保存起来)
    // (例如: AppSession::instance().setCurrentUser(userData); )

    QMessageBox::information(this, "成功", "登录成功!");

    // 跳转到主窗口/查询窗口（TODO）
    // ！！！！！！！！TODO！！！！！！！！！！！
    SearchWindow *sw = new SearchWindow(this); // 这个窗口还没做。
    sw->show();
    this->close(); // 关闭登录窗口
}

// 登录失败的slot函数
void MainWindow::onLoginFailed(const QString& message)
{
    QMessageBox::warning(this, "失败", message);
}
