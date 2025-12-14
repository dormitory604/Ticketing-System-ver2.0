/*构建登陆窗口*/
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "network_manager.h"
#include "register_window.h"
#include "search_window.h"
#include "app_session.h"

#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_registerWindow(nullptr), m_searchWindow(nullptr)
{
    ui->setupUi(this);

    // 绑定 NetworkManager 的信号到本窗口的槽函数
    connect(&NetworkManager::instance(), &NetworkManager::connected,
            this, &MainWindow::onConnected);
    connect(&NetworkManager::instance(), &NetworkManager::tagRegistered,
            this, &MainWindow::onTagRegistered);
    connect(&NetworkManager::instance(), &NetworkManager::tagRegistrationFailed,
            this, &MainWindow::onTagRegistrationFailed);
    connect(&NetworkManager::instance(), &NetworkManager::loginSuccess,
            this, &MainWindow::onLoginSuccess);
    connect(&NetworkManager::instance(), &NetworkManager::loginFailed,
            this, &MainWindow::onLoginFailed);
    connect(&NetworkManager::instance(), &NetworkManager::generalError,
            this, &MainWindow::onGeneralError);
}

MainWindow::~MainWindow()
{
    if (m_registerWindow)
    {
        m_registerWindow->deleteLater();
    }
    if (m_searchWindow)
    {
        m_searchWindow->deleteLater();
    }
    delete ui;
}

// 当用户点击“登录”按钮时
void MainWindow::on_loginButton_clicked()
{
    // 检查tag是否已注册
    if (!NetworkManager::instance().isTagRegistered())
    {
        QMessageBox::warning(this, "连接中", "正在与服务器建立连接，请稍后再试...");
        return;
    }

    QString user = ui->usernameLineEdit->text();
    QString pass = ui->passwordLineEdit->text();

    // 调用 NetworkManager 发送请求
    NetworkManager::instance().sendLoginRequest(user, pass);

    // (这里可以加一个 loading 动画之类的（但是似乎也不会loading太久）)
}

// 登录成功的slot函数
void MainWindow::onLoginSuccess(const QJsonObject &userData)
{
    // (userData 里有 user_id, is_admin 等信息, 可以保存起来)
    AppSession::instance().setCurrentUser(userData);

    QMessageBox::information(this, "成功", "登录成功!");

    if (!m_searchWindow)
    {
        m_searchWindow = new SearchWindow();
    }
    m_searchWindow->show();
    this->hide(); // 登录成功后隐藏登录窗口
}

// 登录失败的slot函数
void MainWindow::onLoginFailed(const QString &message)
{
    QMessageBox::warning(this, "失败", message);
}

void MainWindow::on_registerButton_clicked()
{
    if (!m_registerWindow)
    {
        m_registerWindow = new RegisterWindow(this);
    }
    m_registerWindow->show();
    m_registerWindow->raise();
    m_registerWindow->activateWindow();
}

void MainWindow::onGeneralError(const QString &message)
{
    QMessageBox::critical(this, "网络错误", message);
}

// Tag注册相关槽函数
void MainWindow::onConnected()
{
    qDebug() << "客户端已连接到服务器，等待tag注册...";
}

void MainWindow::onTagRegistered()
{
    qDebug() << "Tag注册成功，可以开始使用业务功能";
}

void MainWindow::onTagRegistrationFailed(const QString &message)
{
    QMessageBox::critical(this, "连接错误", QString("Tag注册失败: %1").arg(message));
}
