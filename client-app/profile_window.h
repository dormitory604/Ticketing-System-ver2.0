#ifndef PROFILE_WINDOW_H
#define PROFILE_WINDOW_H

#include <QDialog>
#include <QJsonObject>
#include <QShowEvent>

namespace Ui {
class ProfileWindow;
}

class ProfileWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileWindow(QWidget *parent = nullptr);
    ~ProfileWindow();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void on_saveButton_clicked();
    void on_closeButton_clicked();

    void handleProfileUpdateSuccess(const QString& message, const QJsonObject& userData);
    void handleProfileUpdateFailed(const QString& message);
    void handleGeneralError(const QString& message);
    void handleUserChanged(const QJsonObject& user);

private:
    void populateUserInfo(const QJsonObject& user);
    bool validateInputs(QString& username, QString& password) const;

    Ui::ProfileWindow *ui;
};

#endif // PROFILE_WINDOW_H
