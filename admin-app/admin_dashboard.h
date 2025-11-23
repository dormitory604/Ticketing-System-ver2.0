#ifndef ADMIN_DASHBOARD_H
#define ADMIN_DASHBOARD_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class AdminDashboard;
}
QT_END_NAMESPACE

class AdminDashboard : public QMainWindow
{
    Q_OBJECT

public:
    AdminDashboard(QWidget *parent = nullptr);
    ~AdminDashboard();

private:
    Ui::AdminDashboard *ui;
};
#endif // ADMIN_DASHBOARD_H
