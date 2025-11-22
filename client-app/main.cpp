#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

#include "network_manager.h" // 1. 包含

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 尝试连接服务器
    // (localhost = 127.0.0.1, 端口必须和服务器一致)
    NetworkManager::instance().connectToServer("127.0.0.1", 12345);

    // 下面这个是自动翻译部分，由Qt提供，可以将软件自动翻译成英文版本。
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "client-app_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    MainWindow w;
    w.show();
    return a.exec();
}
