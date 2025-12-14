#ifndef REGISTER_WINDOW_H
#define REGISTER_WINDOW_H

#include <QDialog>
#include <QShowEvent>

namespace Ui {
class RegisterWindow;
}

class RegisterWindow : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterWindow(QWidget *parent = nullptr);
    ~RegisterWindow();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void on_registerButton_clicked();
    void on_cancelButton_clicked();
    void handleRegisterSuccess(const QString& message);
    void handleRegisterFailed(const QString& message);

private:
    void resetForm();
    void setSubmitting(bool submitting);

    Ui::RegisterWindow *ui;
    bool m_isSubmitting{false};
};

#endif // REGISTER_WINDOW_H
