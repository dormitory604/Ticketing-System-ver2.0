/*
该程序负责开启服务器端服务
*/

#include "database_manager.h"
#include "tcp_server.h"

int main(int argc, char *argv[])
{
    // Set up code that uses the Qt event loop here.
    // Call a.quit() or a.exit() to quit the application.
    // A not very useful example would be including
    // #include <QTimer>
    // near the top of the file and calling
    // QTimer::singleShot(5000, &a, &QCoreApplication::quit);
    // which quits the application after 5 seconds.

    // If you do not need a running Qt event loop, remove the call
    // to a.exec() or use the Non-Qt Plain C++ Application template.


    // 使用 QCoreApplication（而不是 QApplication）
    QCoreApplication a(argc, argv);

    qInfo() << "服务器启动中...";

    if (!DatabaseManager::instance().init()) {
        qCritical() << "数据库初始化失败，服务器退出。";
        return -1;
    }

    // 创建并启动 TCP 服务器
    TcpServer server;
    server.startServer(12345); // 监听 12345 端口

    return a.exec();
}
