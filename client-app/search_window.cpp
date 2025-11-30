#include "search_window.h"
#include "ui_search_window.h"

#include "network_manager.h"
#include "app_session.h"
#include "myorders_window.h"
#include "profile_window.h"

#include <QMessageBox>
#include <QDate>
#include <QShowEvent>
#include <QTableWidgetItem>

SearchWindow::SearchWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SearchWindow)
    , m_ordersWindow(nullptr)
    , m_profileWindow(nullptr)
{
    ui->setupUi(this);
    ui->dateEdit->setDate(QDate::currentDate());
    ui->flightsTable->setColumnCount(7);
    QStringList headers{tr("航班号"), tr("出发地"), tr("目的地"), tr("起飞时间"), tr("到达时间"), tr("剩余座位"), tr("价格")};
    ui->flightsTable->setHorizontalHeaderLabels(headers);
    ui->flightsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->flightsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->flightsTable->horizontalHeader()->setStretchLastSection(true);

    connect(&NetworkManager::instance(), &NetworkManager::searchResults,
            this, &SearchWindow::handleSearchResults);
    connect(&NetworkManager::instance(), &NetworkManager::bookingSuccess,
            this, &SearchWindow::handleBookingSuccess);
    connect(&NetworkManager::instance(), &NetworkManager::bookingFailed,
            this, &SearchWindow::handleBookingFailed);
    connect(&NetworkManager::instance(), &NetworkManager::generalError,
            this, &SearchWindow::handleGeneralError);
}

SearchWindow::~SearchWindow()
{
    if (m_ordersWindow) {
        m_ordersWindow->deleteLater();
    }
    if (m_profileWindow) {
        m_profileWindow->deleteLater();
    }
    delete ui;
}

void SearchWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    updateUserSummary();
}

void SearchWindow::on_searchButton_clicked()
{
    const QString origin = ui->originLineEdit->text().trimmed();
    const QString dest = ui->destinationLineEdit->text().trimmed();
    const QString date = ui->dateEdit->date().toString("yyyy-MM-dd");

    NetworkManager::instance().sendSearchRequest(origin, dest, date);
    ui->statusLabel->setText(tr("正在查询航班..."));
}

void SearchWindow::on_bookButton_clicked()
{
    const int flightId = currentSelectedFlightId();
    if (flightId <= 0) {
        QMessageBox::warning(this, tr("提示"), tr("请先选择要预订的航班"));
        return;
    }

    const int userId = AppSession::instance().userId();
    if (userId <= 0) {
        QMessageBox::warning(this, tr("提示"), tr("请先重新登录"));
        return;
    }

    NetworkManager::instance().bookFlightRequest(userId, flightId);
    ui->statusLabel->setText(tr("正在提交预订请求..."));
}

void SearchWindow::on_myOrdersButton_clicked()
{
    if (!m_ordersWindow) {
        m_ordersWindow = new MyOrdersWindow(this);
    }
    m_ordersWindow->show();
    m_ordersWindow->raise();
    m_ordersWindow->activateWindow();
}

void SearchWindow::on_profileButton_clicked()
{
    if (!m_profileWindow) {
        m_profileWindow = new ProfileWindow(this);
    }
    m_profileWindow->show();
    m_profileWindow->raise();
    m_profileWindow->activateWindow();
}

void SearchWindow::handleSearchResults(const QJsonArray &flights)
{
    m_latestFlights = flights;
    populateFlightsTable(flights);
    ui->statusLabel->setText(tr("查询完成，共 %1 条结果").arg(flights.size()));
}

void SearchWindow::handleBookingSuccess(const QJsonObject &bookingData)
{
    QMessageBox::information(this, tr("预订成功"), tr("订单编号：%1").arg(bookingData.value("booking_id").toInt()));
    ui->statusLabel->setText(tr("预订成功"));
}

void SearchWindow::handleBookingFailed(const QString &message)
{
    QMessageBox::critical(this, tr("预订失败"), message);
    ui->statusLabel->setText(message);
}

void SearchWindow::handleGeneralError(const QString &message)
{
    QMessageBox::critical(this, tr("错误"), message);
    ui->statusLabel->setText(message);
}

void SearchWindow::populateFlightsTable(const QJsonArray &flights)
{
    ui->flightsTable->setRowCount(flights.size());
    for (int row = 0; row < flights.size(); ++row) {
        const QJsonObject obj = flights.at(row).toObject();
        ui->flightsTable->setItem(row, 0, new QTableWidgetItem(obj.value("flight_number").toString()));
        ui->flightsTable->setItem(row, 1, new QTableWidgetItem(obj.value("origin").toString()));
        ui->flightsTable->setItem(row, 2, new QTableWidgetItem(obj.value("destination").toString()));
        ui->flightsTable->setItem(row, 3, new QTableWidgetItem(obj.value("departure_time").toString()));
        ui->flightsTable->setItem(row, 4, new QTableWidgetItem(obj.value("arrival_time").toString()));
        ui->flightsTable->setItem(row, 5, new QTableWidgetItem(QString::number(obj.value("remaining_seats").toInt())));
        ui->flightsTable->setItem(row, 6, new QTableWidgetItem(QString::number(obj.value("price").toDouble(), 'f', 2)));
    }
    ui->flightsTable->resizeColumnsToContents();
}

int SearchWindow::currentSelectedFlightId() const
{
    const QModelIndexList selected = ui->flightsTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return -1;
    }

    const int row = selected.first().row();
    if (row < 0 || row >= m_latestFlights.size()) {
        return -1;
    }
    const QJsonObject obj = m_latestFlights.at(row).toObject();
    return obj.value("flight_id").toInt();
}

void SearchWindow::updateUserSummary()
{
    if (AppSession::instance().isLoggedIn()) {
        ui->userSummaryLabel->setText(tr("当前用户：%1").arg(AppSession::instance().username()));
    } else {
        ui->userSummaryLabel->setText(tr("未登录"));
    }
}
