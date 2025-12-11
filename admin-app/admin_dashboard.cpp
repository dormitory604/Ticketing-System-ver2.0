#include "admin_dashboard.h"
#include "ui_admin_dashboard.h"
#include "network_manager.h"
#include "flightdialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>
#include <QTimer>

AdminDashboard::AdminDashboard(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::AdminDashboard)
{
    ui->setupUi(this);

    // 初始化表格样式（设置列头）
    setupTables();

    // 连接NetworkManager的信号
    NetworkManager *nm = &NetworkManager::instance();

    // 当收到航班列表，调用updateFlightTable函数
    connect(nm, &NetworkManager::allFlightsReceived, this, &AdminDashboard::updateFlightTable);
    // 当收到用户列表，调用updateUserTable函数
    connect(nm, &NetworkManager::allUsersReceived, this, &AdminDashboard::updateUserTable);
    // 当收到订单列表，调用updateBookingTable函数
    connect(nm, &NetworkManager::allBookingsReceived, this, &AdminDashboard::updateBookingTable);

    // 当操作成功/失败，弹窗提示
    connect(nm, &NetworkManager::adminOperationSuccess, this, &AdminDashboard::handleOperationSuccess);
    connect(nm, &NetworkManager::adminOperationFailed, this, &AdminDashboard::handleOperationFailed);

    // 窗口一打开，立刻模拟点击“刷新”按钮，拉取数据
    on_btnRefresh_clicked();
}

AdminDashboard::~AdminDashboard()
{
    delete ui;
}

// 辅助函数：设置表头
void AdminDashboard::setupTables()
{
    // 设置航班表头
    ui->flightTable->setColumnCount(9);
    ui->flightTable->setHorizontalHeaderLabels({"ID", "航班号", "机型", "出发地", "目的地", "起飞时间", "到达时间", "价格", "总座/余票"});
    ui->flightTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); // 自适应宽度填满窗口
    ui->flightTable->setEditTriggers(QAbstractItemView::NoEditTriggers);  // 禁止直接双击表格编辑，只读
    ui->flightTable->setSelectionBehavior(QAbstractItemView::SelectRows);  // 点击时选中一整行

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

// 点击“刷新”按钮
void AdminDashboard::on_btnRefresh_clicked()
{
    // 马上查航班
    NetworkManager::instance().sendGetAllFlightsRequest();

    // 延迟200毫秒查用户
    QTimer::singleShot(200, [this]()
    {
        NetworkManager::instance().sendAdminGetAllUsersRequest();
    });

    // 延迟400毫秒查订单
    QTimer::singleShot(400, [this]()
    {
        NetworkManager::instance().sendAdminGetAllBookingsRequest();
    });
}

// 点击“删除”按钮
void AdminDashboard::on_btnDeleteFlight_clicked()
{
    // 获取当前选中的行
    int currentRow = ui->flightTable->currentRow();
    if (currentRow < 0)
    {
        QMessageBox::warning(this, "提示", "请先选择要删除的航班！");
        return;
    }

    // 获取该行第一列（ID列）的内容
    int flightId = ui->flightTable->item(currentRow, 0)->text().toInt();

    // 二次确认
    if (QMessageBox::question(this, "确认", "确定要删除该航班吗？") == QMessageBox::Yes)
    {
        // 发送删除请求
        NetworkManager::instance().sendAdminDeleteFlightRequest(flightId);
    }
}

// 添加航班
void AdminDashboard::on_btnAddFlight_clicked()
{
    // 创建弹窗
    FlightDialog dlg(this);

    // 显示弹窗并等待用户点击“确定”或“取消”
    if (dlg.exec() == QDialog::Accepted)
    {
        // 点击“确定”，获取数据
        QJsonObject newFlightData = dlg.getFlightData();

        // 发送给服务器
        NetworkManager::instance().sendAdminAddFlightRequest(newFlightData);
    }
}

// 编辑航班
void AdminDashboard::on_btnEditFlight_clicked()
{
    // 获取当前选中的行
    int currentRow = ui->flightTable->currentRow();
    if (currentRow < 0)
    {
        QMessageBox::warning(this, "提示", "请先选择要修改的航班！");
        return;
    }

    // 从表格中提取当前航班的数据
    // 0:ID, 1:航班号, 2:出发, 3:目的, 4:时间, 5:价格, 6:座位
    int id = ui->flightTable->item(currentRow, 0)->text().toInt();
    QString no = ui->flightTable->item(currentRow, 1)->text();
    QString model = ui->flightTable->item(currentRow, 2)->text();
    QString origin = ui->flightTable->item(currentRow, 3)->text();
    QString dest = ui->flightTable->item(currentRow, 4)->text();
    QDateTime deptTime = QDateTime::fromString(ui->flightTable->item(currentRow, 5)->text(), "yyyy-MM-dd HH:mm:ss");
    QDateTime arrTime = QDateTime::fromString(ui->flightTable->item(currentRow, 6)->text(), "yyyy-MM-dd HH:mm:ss");
    double price = ui->flightTable->item(currentRow, 7)->text().toDouble();
    QString seatsStr = ui->flightTable->item(currentRow, 8)->text();
    // 解析最后一列 "总座/余票"，假设显示格式为 "100 / 50"
    QStringList parts = seatsStr.split("/");
    int totalSeats = 0;
    int remainingSeats = 0;
    if(parts.size() >= 1) totalSeats = parts[0].trimmed().toInt();
    if(parts.size() >= 2) remainingSeats = parts[1].trimmed().toInt();

    // 创建弹窗并填入旧数据
    FlightDialog dlg(this);
    dlg.setFlightData(id, no, model, origin, dest, deptTime, arrTime, price, totalSeats, remainingSeats);

    // 显示弹窗
    if (dlg.exec() == QDialog::Accepted)
    {
        // 获取修改后的数据
        QJsonObject updatedData = dlg.getFlightData();
        // 从JSON里单独把ID拿出来
        int flightId = updatedData["flight_id"].toInt();
        // 发送更新请求
        NetworkManager::instance().sendAdminUpdateFlightRequest(flightId, updatedData);
    }
}

// 航班数据更新槽函数（接收服务器数据并填表）
void AdminDashboard::updateFlightTable(const QJsonArray &flights)
{
    ui->flightTable->setRowCount(0);  // 清空旧数据

    // 遍历JSON数组
    for (const QJsonValue &val : flights)
    {

        QJsonObject obj = val.toObject();  //拿到单条航班信息
        int row = ui->flightTable->rowCount();  // 获取行数
        ui->flightTable->insertRow(row);  // 末尾新建一行

        // 填入数据
        ui->flightTable->setItem(row, 0, new QTableWidgetItem(QString::number(obj["flight_id"].toInt())));
        ui->flightTable->setItem(row, 1, new QTableWidgetItem(obj["flight_number"].toString()));
        ui->flightTable->setItem(row, 2, new QTableWidgetItem(obj["model"].toString()));
        ui->flightTable->setItem(row, 3, new QTableWidgetItem(obj["origin"].toString()));
        ui->flightTable->setItem(row, 4, new QTableWidgetItem(obj["destination"].toString()));
        ui->flightTable->setItem(row, 5, new QTableWidgetItem(obj["departure_time"].toString()));
        ui->flightTable->setItem(row, 6, new QTableWidgetItem(obj["arrival_time"].toString()));
        ui->flightTable->setItem(row, 7, new QTableWidgetItem(QString::number(obj["price"].toDouble())));
        // 合并显示总座和余票
        int total = obj["total_seats"].toInt();
        int remain = obj["remaining_seats"].toInt();
        ui->flightTable->setItem(row, 8, new QTableWidgetItem(QString("%1 / %2").arg(total).arg(remain)));
    }
}

// 用户数据更新槽函数（接收服务器数据并填表）
void AdminDashboard::updateUserTable(const QJsonArray &users)
{
    ui->userTable->setRowCount(0);  // 清空旧数据

    // 遍历JSON数组
    for (const QJsonValue &val : users)
    {
        QJsonObject obj = val.toObject();  // 拿到单条用户信息
        int row = ui->userTable->rowCount();  // 获取行数
        ui->userTable->insertRow(row);  // 末尾新建一行

        // 填入数据
        ui->userTable->setItem(row, 0, new QTableWidgetItem(QString::number(obj["user_id"].toInt())));
        ui->userTable->setItem(row, 1, new QTableWidgetItem(obj["username"].toString()));
        ui->userTable->setItem(row, 2, new QTableWidgetItem(obj["created_at"].toString()));
    }
}

// 订单数据更新槽函数（接收服务器数据并填表）
void AdminDashboard::updateBookingTable(const QJsonArray &bookings)
{
    ui->bookingTable->setRowCount(0);  // 清空旧数据

    // 遍历JSON数组
    for (const QJsonValue &val : bookings)
    {
        QJsonObject obj = val.toObject();  // 拿到单条订单信息
        int row = ui->bookingTable->rowCount();  // 获取行数
        ui->bookingTable->insertRow(row);  // 末尾新建一行

        // 填入数据
        ui->bookingTable->setItem(row, 0, new QTableWidgetItem(QString::number(obj["booking_id"].toInt())));
        ui->bookingTable->setItem(row, 1, new QTableWidgetItem(QString::number(obj["user_id"].toInt())));
        ui->bookingTable->setItem(row, 2, new QTableWidgetItem(QString::number(obj["flight_id"].toInt())));
        ui->bookingTable->setItem(row, 3, new QTableWidgetItem(obj["status"].toString()));
    }
}

// 提示操作成功
void AdminDashboard::handleOperationSuccess(const QString &msg)
{
    QMessageBox::information(this, "成功", msg);
    // 操作成功后自动刷新一下列表
    on_btnRefresh_clicked();
}

// 提示操作失败
void AdminDashboard::handleOperationFailed(const QString &msg)
{
    QMessageBox::warning(this, "失败", msg);
}
