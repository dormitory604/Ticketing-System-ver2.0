/*
该程序负责建立和管理数据库
注意，所有需要使用数据库的部分，都需要通过调用该文件当中的DatabaseManager类来实现。
也就是说，在server-app的任何地方，如果你需要调用数据库，请使用DatabaseManager::instance().database()
在main.cpp中初始化该数据库
*/
#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>

class DatabaseManager {
public:
    // 这里是确保整个程序只有唯一的实例
    static DatabaseManager& instance() {
        static DatabaseManager instance;
        return instance;
    }

    // 初始化数据库
    bool init() {
        m_db = QSqlDatabase::addDatabase("QSQLITE");

        QString dbPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

        QDir().mkpath(dbPath);
        m_db.setDatabaseName(dbPath + "/flight_system.db");


        if (!m_db.open())
        {
            qCritical() << "数据库打开失败:" << m_db.lastError().text();
            return false;
        }

        qInfo() << "数据库连接成功! 路径:" << dbPath + "/flight_system.db";

        // 启用外键约束（SQLlite默认是没有开启外键约束的
        QSqlQuery query(m_db);
        if (!query.exec("PRAGMA foreign_keys = ON;"))
        {
            qWarning() << "启用外键失败:" << query.lastError().text();
        }

        return createTables();
    }

    // 提供一个公共访问接口，允许其他类获得QSqlDatabase对象以使用SQL语句进行查询
    QSqlDatabase database() { return m_db; }

private:
    // 私有构造函数，防止外部创建实例
    DatabaseManager() {}
    ~DatabaseManager() { m_db.close(); }
    // 禁用拷贝（这两步都是为了保证这是唯一实例）
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    // 表的具体形式，可以去查看共享文档
    bool createTables() {
        QSqlQuery query(m_db);

        // 创建 User 表
        if (!query.exec("CREATE TABLE IF NOT EXISTS User ("
                        "user_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                        "username TEXT NOT NULL UNIQUE,"
                        "password TEXT NOT NULL,"
                        "is_admin INTEGER NOT NULL DEFAULT 0,"
                        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
                        ");")) {
            qCritical() << "创建User表失败:" << query.lastError().text();
            return false;
        }

        // 创建 Flight 表
        if (!query.exec("CREATE TABLE IF NOT EXISTS Flight ("
                        "flight_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                        "flight_number TEXT NOT NULL,"
                        "model TEXT,"
                        "origin TEXT NOT NULL,"
                        "destination TEXT NOT NULL,"
                        "departure_time DATETIME NOT NULL,"
                        "arrival_time DATETIME NOT NULL,"
                        "total_seats INTEGER NOT NULL,"
                        "remaining_seats INTEGER NOT NULL,"
                        "price REAL NOT NULL,"
                        "is_deleted INTEGER NOT NULL DEFAULT 0"
                        ");")) {
            qCritical() << "创建Flight表失败:" << query.lastError().text();
            return false;
        }

        // 创建 Booking 表
        if (!query.exec("CREATE TABLE IF NOT EXISTS Booking ("
                        "booking_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                        "user_id INTEGER NOT NULL,"
                        "flight_id INTEGER NOT NULL,"
                        "booking_time DATETIME DEFAULT CURRENT_TIMESTAMP,"
                        "status TEXT NOT NULL,"
                        "FOREIGN KEY (user_id) REFERENCES User (user_id),"
                        "FOREIGN KEY (flight_id) REFERENCES Flight (flight_id)"
                        ");")) {
            qCritical() << "创建Booking表失败:" << query.lastError().text();
            return false;
        }

        qInfo() << "所有表检查/创建成功!";

        // 插入一个默认管理员账户，方便测试
        // 这里的SQL写法也确保了这个管理员帐户不会被反复插入。
        query.exec("INSERT INTO User (username, password, is_admin) "
                   "SELECT 'jaisonZheng', 'admin123', 1 "
                   "WHERE NOT EXISTS (SELECT 1 FROM User WHERE username = 'jaisonZheng');");

        return true;
    }

    QSqlDatabase m_db;
};

#endif // DATABASE_MANAGER_H
