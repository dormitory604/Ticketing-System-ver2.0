#ifndef ADMIN_DASHBOARD_H
#define ADMIN_DASHBOARD_H

#include <QMainWindow>
#include <QJsonArray>
#include <QJsonObject>

QT_BEGIN_NAMESPACE
namespace Ui { class AdminDashboard; }
QT_END_NAMESPACE

class AdminDashboard : public QMainWindow
{
    Q_OBJECT

public:
    AdminDashboard(QWidget *parent = nullptr);
    ~AdminDashboard();

private slots:
    // admin_dashboard.ui中的按钮
    void on_btnAddFlight_clicked();  // 点击添加
    void on_btnDeleteFlight_clicked();  // 点击删除
    void on_btnEditFlight_clicked();  // 点击修改
    void on_btnRefresh_clicked();  // 点击刷新
    void on_btnSearchFlight_clicked(); // 搜索按钮
    void on_btnPrevPage_clicked();   // 上一页
    void on_btnNextPage_clicked();   // 下一页

    // NetworkManager信号接收槽
    void updateFlightTable(const QJsonArray &flights);  // 填航班表
    void updateUserTable(const QJsonArray &users);  // 填用户表
    void updateBookingTable(const QJsonArray &bookings);  // 填订单表

    // 处理操作成功/失败的弹窗
    void handleOperationSuccess(const QString &msg);
    void handleOperationFailed(const QString &msg);

private:
    Ui::AdminDashboard *ui;

    // 航班分页状态变量
    QJsonArray m_allFilteredFlights; // 存储服务器返回的所有（最多1000条）过滤后的航班数据
    int m_currentPage = 1;          // 当前页码
    const int m_pageSize = 50;      // 每页显示的行数
    int m_totalPages = 0;           // 总页数

    // 辅助函数：初始化表格表头
    void setupTables();

    // 辅助函数：根据 m_currentPage 和 m_pageSize 填充表格
    void displayCurrentPageFlights();

    // 样式美化函数
    void applyStyles();
};
#endif // ADMIN_DASHBOARD_H
