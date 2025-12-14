#include "myorders_window.h"
#include "ui_myorders_window.h"

#include "network_manager.h"
#include "app_session.h"

#include <QMessageBox>
#include <QShowEvent>
#include <QTableWidgetItem>

MyOrdersWindow::MyOrdersWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MyOrdersWindow)
{
    ui->setupUi(this);

    ui->ordersTable->setColumnCount(7);
    QStringList headers{tr("订单号"), tr("航班号"), tr("出发地"), tr("目的地"), tr("起飞时间"), tr("状态"), tr("价格")};
    ui->ordersTable->setHorizontalHeaderLabels(headers);
    ui->ordersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->ordersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->ordersTable->horizontalHeader()->setStretchLastSection(true);

    connect(&NetworkManager::instance(), &NetworkManager::myOrdersResult,
            this, &MyOrdersWindow::handleOrdersResult);
        connect(&NetworkManager::instance(), &NetworkManager::myOrdersFailed,
            this, &MyOrdersWindow::handleOrdersFailed);
    connect(&NetworkManager::instance(), &NetworkManager::cancelOrderSuccess,
            this, &MyOrdersWindow::handleCancelSuccess);
    connect(&NetworkManager::instance(), &NetworkManager::cancelOrderFailed,
            this, &MyOrdersWindow::handleCancelFailed);
    connect(&NetworkManager::instance(), &NetworkManager::generalError,
            this, &MyOrdersWindow::handleGeneralError);
}

MyOrdersWindow::~MyOrdersWindow()
{
    delete ui;
}

void MyOrdersWindow::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    requestOrders();
}

void MyOrdersWindow::on_refreshButton_clicked()
{
    requestOrders();
}

void MyOrdersWindow::on_cancelOrderButton_clicked()
{
    const int bookingId = currentBookingId();
    if (bookingId <= 0) {
        QMessageBox::warning(this, tr("提示"), tr("请先选择要取消的订单"));
        return;
    }

    NetworkManager::instance().cancelOrderRequest(bookingId);
    ui->statusLabel->setText(tr("正在取消订单..."));
}

void MyOrdersWindow::handleOrdersResult(const QJsonArray &orders)
{
    m_latestOrders = orders;
    populateOrdersTable(orders);
    ui->statusLabel->setText(tr("共 %1 个订单").arg(orders.size()));
    setLoading(false);
}

void MyOrdersWindow::handleOrdersFailed(const QString &message)
{
    QMessageBox::warning(this, tr("拉取失败"), message);
    ui->statusLabel->setText(message);
    setLoading(false);
}

void MyOrdersWindow::handleCancelSuccess(const QString &message)
{
    QMessageBox::information(this, tr("取消成功"), message);
    setLoading(false);
    requestOrders();
}

void MyOrdersWindow::handleCancelFailed(const QString &message)
{
    QMessageBox::critical(this, tr("取消失败"), message);
    ui->statusLabel->setText(message);
}

void MyOrdersWindow::handleGeneralError(const QString &message)
{
    if (m_loading) {
        QMessageBox::critical(this, tr("错误"), message);
        ui->statusLabel->setText(message);
        setLoading(false);
    }
}

void MyOrdersWindow::requestOrders()
{
    if (m_loading) {
        ui->statusLabel->setText(tr("正在加载订单，请稍候"));
        return;
    }
    const int userId = AppSession::instance().userId();
    if (userId <= 0) {
        QMessageBox::warning(this, tr("提示"), tr("请先登录"));
        return;
    }
    NetworkManager::instance().getMyOrdersRequest(userId);
    ui->statusLabel->setText(tr("正在拉取订单..."));
    setLoading(true);
}

int MyOrdersWindow::currentBookingId() const
{
    const QModelIndexList selected = ui->ordersTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return -1;
    }

    const int row = selected.first().row();
    if (row < 0 || row >= m_latestOrders.size()) {
        return -1;
    }
    const QJsonObject obj = m_latestOrders.at(row).toObject();
    return obj.value("booking_id").toInt();
}

void MyOrdersWindow::populateOrdersTable(const QJsonArray &orders)
{
    ui->ordersTable->setRowCount(orders.size());
    for (int row = 0; row < orders.size(); ++row) {
        const QJsonObject obj = orders.at(row).toObject();
        ui->ordersTable->setItem(row, 0, new QTableWidgetItem(QString::number(obj.value("booking_id").toInt())));
        ui->ordersTable->setItem(row, 1, new QTableWidgetItem(obj.value("flight_number").toString()));
        ui->ordersTable->setItem(row, 2, new QTableWidgetItem(obj.value("origin").toString()));
        ui->ordersTable->setItem(row, 3, new QTableWidgetItem(obj.value("destination").toString()));
        ui->ordersTable->setItem(row, 4, new QTableWidgetItem(obj.value("departure_time").toString()));
        ui->ordersTable->setItem(row, 5, new QTableWidgetItem(obj.value("status").toString()));
        ui->ordersTable->setItem(row, 6, new QTableWidgetItem(QString::number(obj.value("price").toDouble(), 'f', 2)));
    }
    ui->ordersTable->resizeColumnsToContents();
}

void MyOrdersWindow::setLoading(bool loading)
{
    if (m_loading == loading) {
        return;
    }
    m_loading = loading;
    ui->refreshButton->setEnabled(!loading);
}
