#include "admin_dashboard.h"
#include "ui_admin_dashboard.h"
#include "network_manager.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>

AdminDashboard::AdminDashboard(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::AdminDashboard)
{
    ui->setupUi(this);

    // 1. 初始化表格样式（设置列头）
    setupTables();

    // 2. 连接 NetworkManager 的信号
    NetworkManager *nm = &NetworkManager::instance();

    // 当收到航班列表 -> 调用 updateFlightTable
    connect(nm, &NetworkManager::allFlightsReceived, this, &AdminDashboard::updateFlightTable);
    // 当收到用户列表 -> 调用 updateUserTable
    connect(nm, &NetworkManager::allUsersReceived, this, &AdminDashboard::updateUserTable);
    // 当收到订单列表 -> 调用 updateBookingTable
    connect(nm, &NetworkManager::allBookingsReceived, this, &AdminDashboard::updateBookingTable);

    // 当操作成功/失败 -> 弹窗提示
    connect(nm, &NetworkManager::adminOperationSuccess, this, &AdminDashboard::handleOperationSuccess);
    connect(nm, &NetworkManager::adminOperationFailed, this, &AdminDashboard::handleOperationFailed);

    // 3. 窗口一打开，立刻模拟点击“刷新”按钮，去拉取数据
    on_btnRefresh_clicked();
}

AdminDashboard::~AdminDashboard()
{
    delete ui;
}

// ================= 辅助函数：设置表头 =================

void AdminDashboard::setupTables()
{
    // 设置航班表头
    ui->flightTable->setColumnCount(7);
    ui->flightTable->setHorizontalHeaderLabels({"ID", "航班号", "出发地", "目的地", "起飞时间", "价格", "余票"});
    ui->flightTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); // 自适应宽度
    ui->flightTable->setEditTriggers(QAbstractItemView::NoEditTriggers); // 禁止双击编辑
    ui->flightTable->setSelectionBehavior(QAbstractItemView::SelectRows); // 选中整行

    // 设置用户表头
    ui->userTable->setColumnCount(3);
    ui->userTable->setHorizontalHeaderLabels({"ID", "用户名", "创建时间"});
    ui->userTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->userTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->userTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 设置订单表头
    ui->bookingTable->setColumnCount(4);
    ui->bookingTable->setHorizontalHeaderLabels({"订单ID", "用户ID", "航班ID", "状态"});
    ui->bookingTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->bookingTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->bookingTable->setSelectionBehavior(QAbstractItemView::SelectRows);
}

// ================= 按钮点击事件 =================

void AdminDashboard::on_btnRefresh_clicked()
{
    // 发送查询所有航班的请求
    // (因为Server端目前只实现了 handleSearchFlights，所以这个肯定能用)
    NetworkManager::instance().sendGetAllFlightsRequest();

    // 下面这两个请求，因为Server端可能还没写好 handleAdminGetAllUsers，
    // 发送出去可能会收到 "Error: 未知的action"。
    // 你可以先注释掉，等Server队友写好了再解开。
    // NetworkManager::instance().sendAdminGetAllUsersRequest();
    // NetworkManager::instance().sendAdminGetAllBookingsRequest();
}

void AdminDashboard::on_btnDeleteFlight_clicked()
{
    // 1. 获取当前选中的行
    int currentRow = ui->flightTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "提示", "请先选择要删除的航班！");
        return;
    }

    // 2. 获取该行第一列 (ID列) 的内容
    int flightId = ui->flightTable->item(currentRow, 0)->text().toInt();

    // 3. 二次确认
    if (QMessageBox::question(this, "确认", "确定要删除该航班吗？") == QMessageBox::Yes) {
        // 4. 发送删除请求
        NetworkManager::instance().sendAdminDeleteFlightRequest(flightId);
    }
}

void AdminDashboard::on_btnAddFlight_clicked()
{
    QMessageBox::information(this, "提示", "添加功能需要弹出一个新窗口填数据，暂时还没写。");
    // TODO: 创建 AddFlightDialog
}

void AdminDashboard::on_btnEditFlight_clicked()
{
    int currentRow = ui->flightTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "提示", "请先选择要修改的航班！");
        return;
    }
    QMessageBox::information(this, "提示", "修改功能需要弹出一个新窗口，暂时还没写。");
}

// ================= 数据更新槽函数 (接收服务器数据并填表) =================

void AdminDashboard::updateFlightTable(const QJsonArray &flights)
{
    ui->flightTable->setRowCount(0); // 清空旧数据

    for (const QJsonValue &val : flights) {
        QJsonObject obj = val.toObject();

        int row = ui->flightTable->rowCount();
        ui->flightTable->insertRow(row);

        // 填入数据
        ui->flightTable->setItem(row, 0, new QTableWidgetItem(QString::number(obj["flight_id"].toInt())));
        ui->flightTable->setItem(row, 1, new QTableWidgetItem(obj["flight_number"].toString()));
        ui->flightTable->setItem(row, 2, new QTableWidgetItem(obj["origin"].toString()));
        ui->flightTable->setItem(row, 3, new QTableWidgetItem(obj["destination"].toString()));
        ui->flightTable->setItem(row, 4, new QTableWidgetItem(obj["departure_time"].toString()));
        ui->flightTable->setItem(row, 5, new QTableWidgetItem(QString::number(obj["price"].toDouble())));
        ui->flightTable->setItem(row, 6, new QTableWidgetItem(QString::number(obj["remaining_seats"].toInt())));
    }
}

void AdminDashboard::updateUserTable(const QJsonArray &users)
{
    ui->userTable->setRowCount(0);
    for (const QJsonValue &val : users) {
        QJsonObject obj = val.toObject();
        int row = ui->userTable->rowCount();
        ui->userTable->insertRow(row);

        ui->userTable->setItem(row, 0, new QTableWidgetItem(QString::number(obj["user_id"].toInt())));
        ui->userTable->setItem(row, 1, new QTableWidgetItem(obj["username"].toString()));
        ui->userTable->setItem(row, 2, new QTableWidgetItem(obj["created_at"].toString()));
    }
}

void AdminDashboard::updateBookingTable(const QJsonArray &bookings)
{
    ui->bookingTable->setRowCount(0);
    for (const QJsonValue &val : bookings) {
        QJsonObject obj = val.toObject();
        int row = ui->bookingTable->rowCount();
        ui->bookingTable->insertRow(row);

        ui->bookingTable->setItem(row, 0, new QTableWidgetItem(QString::number(obj["booking_id"].toInt())));
        ui->bookingTable->setItem(row, 1, new QTableWidgetItem(QString::number(obj["user_id"].toInt())));
        ui->bookingTable->setItem(row, 2, new QTableWidgetItem(QString::number(obj["flight_id"].toInt())));
        ui->bookingTable->setItem(row, 3, new QTableWidgetItem(obj["status"].toString()));
    }
}

// ================= 操作反馈 =================

void AdminDashboard::handleOperationSuccess(const QString &msg)
{
    QMessageBox::information(this, "成功", msg);
    // 操作成功后（比如删除了航班），自动刷新一下列表
    on_btnRefresh_clicked();
}

void AdminDashboard::handleOperationFailed(const QString &msg)
{
    QMessageBox::warning(this, "失败", msg);
}
