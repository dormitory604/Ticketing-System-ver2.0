#include "admin_dashboard.h"
#include "admin_login_window.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 先实例化登录窗口
    AdminLoginWindow login;

    // 登录成功（handleLoginResult函数调用accept()时），显示管理员页面
    if(login.exec() == QDialog::Accepted)
    {
        AdminDashboard dashboard;
        dashboard.show();

        return a.exec();
    }

    return 0;
}
