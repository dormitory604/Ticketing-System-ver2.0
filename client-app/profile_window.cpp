#include "profile_window.h"
#include "ui_profile_window.h"

#include "app_session.h"
#include "network_manager.h"

#include <QMessageBox>

ProfileWindow::ProfileWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ProfileWindow)
{
    ui->setupUi(this);

    connect(&NetworkManager::instance(), &NetworkManager::profileUpdateSuccess,
            this, &ProfileWindow::handleProfileUpdateSuccess);
    connect(&NetworkManager::instance(), &NetworkManager::profileUpdateFailed,
            this, &ProfileWindow::handleProfileUpdateFailed);
    connect(&NetworkManager::instance(), &NetworkManager::generalError,
            this, &ProfileWindow::handleGeneralError);
    connect(&AppSession::instance(), &AppSession::userChanged,
            this, &ProfileWindow::handleUserChanged);

    populateUserInfo(AppSession::instance().currentUser());
}

ProfileWindow::~ProfileWindow()
{
    delete ui;
}

void ProfileWindow::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    populateUserInfo(AppSession::instance().currentUser());
    ui->passwordLineEdit->clear();
    ui->confirmPasswordLineEdit->clear();
    ui->statusLabel->setText(tr("在此修改您的个人信息。"));
}

void ProfileWindow::on_saveButton_clicked()
{
    const int userId = AppSession::instance().userId();
    if (userId <= 0) {
        QMessageBox::warning(this, tr("提示"), tr("请先登录"));
        return;
    }

    QString username = ui->usernameLineEdit->text().trimmed();
    QString password = ui->passwordLineEdit->text();
    QString confirm = ui->confirmPasswordLineEdit->text();

    if (!validateInputs(username, password)) {
        return;
    }
    if (!password.isEmpty() && password != confirm) {
        QMessageBox::warning(this, tr("提示"), tr("两次输入的密码不一致"));
        return;
    }

    NetworkManager::instance().updateProfileRequest(userId, username, password);
    ui->statusLabel->setText(tr("正在提交修改..."));
}

void ProfileWindow::on_closeButton_clicked()
{
    close();
}

void ProfileWindow::handleProfileUpdateSuccess(const QString &message, const QJsonObject &userData)
{
    QMessageBox::information(this, tr("修改成功"), message);
    AppSession::instance().setCurrentUser(userData);
    ui->statusLabel->setText(message);
    ui->passwordLineEdit->clear();
    ui->confirmPasswordLineEdit->clear();
}

void ProfileWindow::handleProfileUpdateFailed(const QString &message)
{
    QMessageBox::critical(this, tr("修改失败"), message);
    ui->statusLabel->setText(message);
}

void ProfileWindow::handleGeneralError(const QString &message)
{
    QMessageBox::critical(this, tr("网络错误"), message);
    ui->statusLabel->setText(message);
}

void ProfileWindow::handleUserChanged(const QJsonObject &user)
{
    populateUserInfo(user);
}

void ProfileWindow::populateUserInfo(const QJsonObject &user)
{
    if (user.isEmpty()) {
        ui->summaryLabel->setText(tr("当前用户：-"));
        ui->userIdValueLabel->setText("-");
        ui->usernameLineEdit->clear();
        return;
    }

    ui->summaryLabel->setText(tr("当前用户：%1").arg(user.value("username").toString()));
    ui->userIdValueLabel->setText(QString::number(user.value("user_id").toInt()));
    ui->usernameLineEdit->setText(user.value("username").toString());
}

bool ProfileWindow::validateInputs(QString &username, QString &password) const
{
    if (username.isEmpty()) {
        QMessageBox::warning(nullptr, tr("提示"), tr("用户名不能为空"));
        return false;
    }
    if (!password.isEmpty() && password.length() < 6) {
        QMessageBox::warning(nullptr, tr("提示"), tr("密码至少需要 6 位"));
        return false;
    }
    return true;
}
