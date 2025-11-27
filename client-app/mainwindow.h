#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QJsonObject>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class RegisterWindow;
class SearchWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_loginButton_clicked();
    void on_registerButton_clicked();
    void onLoginSuccess(const QJsonObject& userData);
    void onLoginFailed(const QString& message);
    void onGeneralError(const QString& message);

private:
    Ui::MainWindow *ui;
    RegisterWindow *m_registerWindow;
    SearchWindow *m_searchWindow;
};
#endif // MAINWINDOW_H
