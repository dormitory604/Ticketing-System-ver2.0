#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include "qml_bridge.h"
#include "network_manager.h"
#include "app_session.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // 设置应用信息
    app.setApplicationName("SkyTravel");
    app.setOrganizationName("Ticketing System");
    
    // 可以设置样式，例如 Material
    // QQuickStyle::setStyle("Material");
    
    // 创建 QML 桥接对象
    QmlBridge bridge;
    
    // 连接服务器
    NetworkManager::instance().connectToServer("127.0.0.1", 12345);
    
    // 创建 QML 引擎
    QQmlApplicationEngine engine;
    
    // 将桥接对象注册到 QML 上下文
    engine.rootContext()->setContextProperty("qmlBridge", &bridge);
    
    // 加载主 QML 文件
    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    
    engine.load(url);

    return app.exec();
}


