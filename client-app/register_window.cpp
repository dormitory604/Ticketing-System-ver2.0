#include "register_window.h"
#include "ui_register_window.h"
#include "network_manager.h"

#include <QMessageBox>
#include <QShowEvent>

RegisterWindow::RegisterWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RegisterWindow)
{
    ui->setupUi(this);

    connect(&NetworkManager::instance(), &NetworkManager::registerSuccess,
            this, &RegisterWindow::handleRegisterSuccess);
    connect(&NetworkManager::instance(), &NetworkManager::registerFailed,
            this, &RegisterWindow::handleRegisterFailed);
    connect(&NetworkManager::instance(), &NetworkManager::generalError,
            this, [this](const QString& message) {
                if (m_isSubmitting) {
                    handleRegisterFailed(message);
                }
            });
}

RegisterWindow::~RegisterWindow()
{
    delete ui;
}

void RegisterWindow::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    resetForm();
}

void RegisterWindow::on_registerButton_clicked()
{
    if (m_isSubmitting) {
        ui->statusLabel->setText(tr("正在等待服务器响应..."));
        return;
    }

    const QString username = ui->usernameLineEdit->text().trimmed();
    const QString password = ui->passwordLineEdit->text();
    const QString confirm = ui->confirmPasswordLineEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("用户名和密码不能为空"));
        return;
    }
    if (password != confirm) {
        QMessageBox::warning(this, tr("提示"), tr("两次输入的密码不一致"));
        return;
    }

    NetworkManager::instance().sendRegisterRequest(username, password);
    ui->statusLabel->setText(tr("正在提交注册请求..."));
    setSubmitting(true);
}

void RegisterWindow::on_cancelButton_clicked()
{
    setSubmitting(false);
    close();
}

void RegisterWindow::handleRegisterSuccess(const QString &message)
{
    QMessageBox::information(this, tr("注册成功"), message.isEmpty() ? tr("注册成功") : message);
    setSubmitting(false);
    close();
}

void RegisterWindow::handleRegisterFailed(const QString &message)
{
    QMessageBox::critical(this, tr("注册失败"), message);
    ui->statusLabel->setText(message);
    setSubmitting(false);
}

void RegisterWindow::resetForm()
{
    ui->usernameLineEdit->clear();
    ui->passwordLineEdit->clear();
    ui->confirmPasswordLineEdit->clear();
    ui->statusLabel->setText(tr("请输入信息完成注册"));
    setSubmitting(false);
}

void RegisterWindow::setSubmitting(bool submitting)
{
    if (m_isSubmitting == submitting)
        return;

    m_isSubmitting = submitting;
    ui->registerButton->setEnabled(!submitting);
}
