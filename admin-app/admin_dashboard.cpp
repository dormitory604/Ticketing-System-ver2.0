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

    // 应用样式美化
    applyStyles();

    // 设置默认搜索日期和分页按钮状态
    QDate defaultDate = QDate::fromString("2025-12-12", "yyyy-MM-dd");
    ui->dtSearchDate->setDate(defaultDate);
    ui->dtSearchDate->setCalendarPopup(true);
    ui->btnPrevPage->setEnabled(false);
    ui->btnNextPage->setEnabled(false);

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
    ui->flightTable->setColumnCount(10);
    ui->flightTable->setHorizontalHeaderLabels({"ID", "航班号", "机型", "出发地", "目的地", "起飞时间", "到达时间", "价格", "总座/余票", "状态"});

    // 所有列根据内容自动调整宽度
    ui->flightTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    // 让最后一列自动拉伸，填满窗口右边的空白
    ui->flightTable->horizontalHeader()->setStretchLastSection(true);

    ui->flightTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->flightTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 设置用户表头
    ui->userTable->setColumnCount(4);
    ui->userTable->setHorizontalHeaderLabels({"ID", "用户名", "创建时间", "身份"});
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
    // 清空搜索栏，表示获取所有航班
    ui->txtSearchOrigin->clear();
    ui->txtSearchDestination->clear();

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

    // 检查是否已删除
    // 获取状态列的文本
    QString status = ui->flightTable->item(currentRow, 9)->text();
    if (status == "已删除")
    {
        QMessageBox::warning(this, "操作无效", "该航班已删除，无需重复操作！");
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

    // 检查是否已删除，已删除的航班不能修改
    QString status = ui->flightTable->item(currentRow, 9)->text();
    if (status == "已删除")
    {
        QMessageBox::warning(this, "操作无效", "已删除的航班无法修改！");
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
    // 1. 存储所有过滤后的数据
    m_allFilteredFlights = flights;

    // 2. 计算分页状态
    int totalItems = m_allFilteredFlights.size();
    m_totalPages = qCeil((double)totalItems / m_pageSize);

    // 确保当前页码有效
    m_currentPage = 1;
    if (m_totalPages == 0) m_currentPage = 0;

    // 3. 显示当前页
    displayCurrentPageFlights();
}

// 辅助函数：根据 m_currentPage 和 m_pageSize 填充表格
void AdminDashboard::displayCurrentPageFlights()
{
    ui->flightTable->setRowCount(0);  // 清空旧数据

    if (m_allFilteredFlights.isEmpty())
    {
        // 更新分页信息
        ui->lblPageInfo->setText("Page 0/0 (0 条结果)");
        ui->btnPrevPage->setEnabled(false);
        ui->btnNextPage->setEnabled(false);
        return;
    }

    // 计算起始索引和结束索引
    int totalItems = m_allFilteredFlights.size();
    int startIndex = (m_currentPage - 1) * m_pageSize;
    int endIndex = qMin(startIndex + m_pageSize, totalItems);

    // 更新分页信息
    ui->lblPageInfo->setText(QString("Page %1/%2 (%3 条结果)").arg(m_currentPage).arg(m_totalPages).arg(totalItems));

    // 启用/禁用分页按钮
    ui->btnPrevPage->setEnabled(m_currentPage > 1);
    ui->btnNextPage->setEnabled(m_currentPage < m_totalPages);

    // 遍历当前页的 JSON 数据并填充表格
    for (int i = startIndex; i < endIndex; ++i)
    {
        QJsonObject obj = m_allFilteredFlights.at(i).toObject();
        int row = ui->flightTable->rowCount();
        ui->flightTable->insertRow(row);

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

        // 处理 is_deleted 状态
        int isDeleted = obj["is_deleted"].toInt();
        QTableWidgetItem *statusItem;
        if (isDeleted == 1)
        {
            statusItem = new QTableWidgetItem("已删除");
            // 将这一行的所有单元格文字设为灰色
            for (int col = 0; col <= 8; ++col)
            {
                if (ui->flightTable->item(row, col))
                {
                    ui->flightTable->item(row, col)->setForeground(Qt::gray);
                }
            }
            statusItem->setForeground(Qt::gray); // 状态列也设为灰色
        }
        else
        {
            statusItem = new QTableWidgetItem("正常");
            statusItem->setForeground(QColor("#52c41a")); // 正常显示为绿色
        }

        ui->flightTable->setItem(row, 9, statusItem); // 填入第10列
    }

    // 自动调整列宽
    ui->flightTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->flightTable->horizontalHeader()->setStretchLastSection(true);
}

// 搜索航班
void AdminDashboard::on_btnSearchFlight_clicked()
{
    // 1. 从UI获取搜索参数
    QString origin = ui->txtSearchOrigin->text();
    QString destination = ui->txtSearchDestination->text();
    QString date = ui->dtSearchDate->date().toString("yyyy-MM-dd"); // 格式化日期

    // 2. 调用 NetworkManager 发送请求
    NetworkManager::instance().sendSearchFlightsRequest(origin, destination, date);
}

// 上一页
void AdminDashboard::on_btnPrevPage_clicked()
{
    if (m_currentPage > 1)
    {
        m_currentPage--;
        displayCurrentPageFlights();
    }
}

// 下一页
void AdminDashboard::on_btnNextPage_clicked()
{
    if (m_currentPage < m_totalPages)
    {
        m_currentPage++;
        displayCurrentPageFlights();
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

        // 判断身份
        int isAdmin = obj["is_admin"].toInt();
        QTableWidgetItem *roleItem;

        if (isAdmin == 1)
        {
            roleItem = new QTableWidgetItem("管理员");
            // 管理员换成蓝色
            roleItem->setForeground(QColor("#1890ff"));
        }
        else
        {
            roleItem = new QTableWidgetItem("普通用户");
            roleItem->setForeground(Qt::black);
        }
        ui->userTable->setItem(row, 3, roleItem); // 填入第4列
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

        // 1. 获取两个核心状态
        int isFlightDeleted = obj["is_deleted"].toInt(); // 航班是否被删除
        QString dbStatus = obj["status"].toString(); // 数据库里的订单

        QString displayStatus;
        QTableWidgetItem *statusItem;

        // 2. 逻辑判断
        // 优先级 1: 先判断 is_deleted
        if (isFlightDeleted == 1)
        {
            displayStatus = "航班已失效";
            statusItem = new QTableWidgetItem(displayStatus);
            statusItem->setForeground(Qt::red); // 红色高亮，提示异常
        }
        // 优先级 2: 再判断是否 cancelled
        else if (dbStatus == "cancelled")
        {
            displayStatus = "已取消";
            statusItem = new QTableWidgetItem(displayStatus);
            statusItem->setForeground(Qt::gray); // 灰色，表示无效
        }
        // 优先级 3: 最后才是 confirmed
        else
        {
            displayStatus = "已确认"; // confirmed
            statusItem = new QTableWidgetItem(displayStatus);
            statusItem->setForeground(QColor("#52c41a")); // 绿色，正常
        }

        ui->bookingTable->setItem(row, 3, statusItem);
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

//样式美化函数
void AdminDashboard::applyStyles()
{
    // 1. 全局字体与背景
    // 设置一个看起来比较舒服的无衬线字体
    this->setStyleSheet(R"(
        QMainWindow
        {
            background-color: #f0f2f5;
        }
        QWidget
        {
            font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
            font-size: 10pt;
        }
    )");

    // 2. 表格样式
    // 这种写法对应所有 QTableWidget
    QString tableStyle = R"(
        QTableWidget
        {
            background-color: #ffffff;
            border: 1px solid #dcdcdc;
            gridline-color: #e0e0e0;
            selection-background-color: #e6f7ff; /* 选中项浅蓝背景 */
            selection-color: #000000;            /* 选中项黑色文字 */
            outline: none;                       /* 去掉选中虚线框 */
        }
        QTableWidget::item
        {
            padding: 5px; /* 让行高一点，不拥挤 */
        }
        QHeaderView::section
        {
            background-color: #fafafa;
            padding: 5px;
            border: none;
            border-bottom: 1px solid #dcdcdc;
            border-right: 1px solid #dcdcdc;
            font-weight: bold;
            color: #333333;
        }
    )";
    ui->flightTable->setStyleSheet(tableStyle);
    ui->userTable->setStyleSheet(tableStyle);
    ui->bookingTable->setStyleSheet(tableStyle);

    // 开启隔行变色
    ui->flightTable->setAlternatingRowColors(true);
    ui->userTable->setAlternatingRowColors(true);
    ui->bookingTable->setAlternatingRowColors(true);

    // 隐藏垂直表头 (行号)，通常Admin后台不需要显示 1,2,3...
    ui->flightTable->verticalHeader()->setVisible(false);
    ui->userTable->verticalHeader()->setVisible(false);
    ui->bookingTable->verticalHeader()->setVisible(false);


    // 3. 按钮通用样式
    QString btnStyle = R"(
        QPushButton
        {
            border: 1px solid #d9d9d9;
            border-radius: 4px;
            background-color: #ffffff;
            padding: 5px 15px;
            color: #333333;
        }
        QPushButton:hover
        {
            border-color: #40a9ff;
            color: #40a9ff;
        }
        QPushButton:pressed
        {
            background-color: #f0f0f0;
        }
    )";

    // 应用通用样式
    ui->btnRefresh->setStyleSheet(btnStyle);
    ui->btnEditFlight->setStyleSheet(btnStyle);

    // 4. 特殊按钮样式

    // [添加航班] - 蓝色主色调，显眼
    ui->btnAddFlight->setStyleSheet(R"(
        QPushButton
        {
            background-color: #1890ff;
            border: 1px solid #1890ff;
            border-radius: 4px;
            color: white;
            padding: 5px 15px;
            font-weight: bold;
        }
        QPushButton:hover
        {
            background-color: #40a9ff;
        }
        QPushButton:pressed
        {
            background-color: #096dd9;
        }
    )");

    // [删除航班] - 红色警告色，防止误触
    ui->btnDeleteFlight->setStyleSheet(R"(
        QPushButton
        {
            background-color: #fff1f0;
            border: 1px solid #ffa39e;
            border-radius: 4px;
            color: #ff4d4f;
            padding: 5px 15px;
        }
        QPushButton:hover
        {
            background-color: #ff4d4f;
            color: white;
            border-color: #ff4d4f;
        }
        QPushButton:pressed {
            background-color: #cf1322;
        }
    )");

    // 5. TabWidget 样式 (选项卡)
    ui->tabWidget->setStyleSheet(R"(
        QTabWidget::pane
        {
            border: 1px solid #dcdcdc;
            background: white;
            top: -1px;
        }
        QTabBar::tab
        {
            background: #fafafa;
            border: 1px solid #dcdcdc;
            padding: 8px 20px;
            margin-right: 2px;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
        }
        QTabBar::tab:selected
        {
            background: white;
            border-bottom-color: white; /* 看起来和下面连在一起 */
            font-weight: bold;
            color: #1890ff;
        }
        QTabBar::tab:hover
        {
            background: #fff;
        }
    )");
}
