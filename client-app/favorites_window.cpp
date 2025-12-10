#include "favorites_window.h"
#include "ui_favorites_window.h"
#include "network_manager.h"

#include <QDateTime>
#include <QMessageBox>

FavoritesWindow::FavoritesWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::FavoritesWindow)
    , m_userId(-1)
    , m_username("")
{
    ui->setupUi(this);
    
    // Connect to NetworkManager signals
    connect(&NetworkManager::instance(), &NetworkManager::myFavoritesResult,
            this, &FavoritesWindow::handleMyFavoritesResult);
    connect(&NetworkManager::instance(), &NetworkManager::removeFavoriteSuccess,
            this, &FavoritesWindow::handleRemoveFavoriteSuccess);
    connect(&NetworkManager::instance(), &NetworkManager::removeFavoriteFailed,
            this, &FavoritesWindow::handleRemoveFavoriteFailed);
    connect(&NetworkManager::instance(), &NetworkManager::generalError,
            this, &FavoritesWindow::handleGeneralError);
    
    // Setup table headers
    ui->favoritesTableWidget->setColumnCount(7);
    QStringList headers;
    headers << "航班号" << "出发地" << "目的地" << "起飞时间" << "到达时间" << "价格" << "收藏时间";
    ui->favoritesTableWidget->setHorizontalHeaderLabels(headers);
    
    // Resize columns to fit content
    ui->favoritesTableWidget->horizontalHeader()->setStretchLastSection(true);
    
    updateUserSummary();
}

FavoritesWindow::~FavoritesWindow()
{
    delete ui;
}

void FavoritesWindow::setUserId(int userId)
{
    m_userId = userId;
    updateUserSummary();
}

void FavoritesWindow::setUsername(const QString& username)
{
    m_username = username;
    updateUserSummary();
}

void FavoritesWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    
    if (m_userId > 0) {
        NetworkManager::instance().getMyFavoritesRequest(m_userId);
    }
}

void FavoritesWindow::on_backButton_clicked()
{
    this->close();
}

void FavoritesWindow::on_refreshButton_clicked()
{
    if (m_userId > 0) {
        NetworkManager::instance().getMyFavoritesRequest(m_userId);
    }
}

void FavoritesWindow::on_removeFavoriteButton_clicked()
{
    int favoriteId = currentSelectedFavoriteId();
    int flightId = currentSelectedFlightId();
    
    if (favoriteId <= 0 || flightId <= 0) {
        QMessageBox::warning(this, "提示", "请先选择要取消收藏的航班");
        return;
    }
    
    NetworkManager::instance().removeFavoriteRequest(m_userId, flightId);
}

void FavoritesWindow::on_favoritesTableWidget_itemSelectionChanged()
{
    bool hasSelection = ui->favoritesTableWidget->selectedItems().count() > 0;
    ui->removeFavoriteButton->setEnabled(hasSelection);
}

void FavoritesWindow::handleMyFavoritesResult(const QJsonArray& favorites)
{
    m_latestFavorites = favorites;
    populateFavoritesTable(favorites);
}

void FavoritesWindow::handleRemoveFavoriteSuccess(const QString& message)
{
    QMessageBox::information(this, "成功", message);
    // Refresh the favorites list
    on_refreshButton_clicked();
}

void FavoritesWindow::handleRemoveFavoriteFailed(const QString& message)
{
    QMessageBox::warning(this, "失败", message);
}

void FavoritesWindow::handleGeneralError(const QString& message)
{
    QMessageBox::critical(this, "错误", message);
}

void FavoritesWindow::populateFavoritesTable(const QJsonArray& favorites)
{
    ui->favoritesTableWidget->setRowCount(0);
    
    for (int i = 0; i < favorites.size(); ++i) {
        QJsonObject favorite = favorites[i].toObject();
        
        int row = ui->favoritesTableWidget->rowCount();
        ui->favoritesTableWidget->insertRow(row);
        
        // Flight Number
        ui->favoritesTableWidget->setItem(row, 0, new QTableWidgetItem(favorite["flight_number"].toString()));
        
        // Origin
        ui->favoritesTableWidget->setItem(row, 1, new QTableWidgetItem(favorite["origin"].toString()));
        
        // Destination
        ui->favoritesTableWidget->setItem(row, 2, new QTableWidgetItem(favorite["destination"].toString()));
        
        // Departure Time
        QString departureTime = favorite["departure_time"].toString();
        QDateTime departureDateTime = QDateTime::fromString(departureTime, Qt::ISODate);
        ui->favoritesTableWidget->setItem(row, 3, new QTableWidgetItem(departureDateTime.toString("MM-dd hh:mm")));
        
        // Arrival Time
        QString arrivalTime = favorite["arrival_time"].toString();
        QDateTime arrivalDateTime = QDateTime::fromString(arrivalTime, Qt::ISODate);
        ui->favoritesTableWidget->setItem(row, 4, new QTableWidgetItem(arrivalDateTime.toString("MM-dd hh:mm")));
        
        // Price
        double price = favorite["price"].toDouble();
        ui->favoritesTableWidget->setItem(row, 5, new QTableWidgetItem(QString("¥%1").arg(price, 0, 'f', 0)));
        
        // Created At (收藏时间)
        QString createdAt = favorite["created_at"].toString();
        QDateTime createdDateTime = QDateTime::fromString(createdAt, Qt::ISODate);
        ui->favoritesTableWidget->setItem(row, 6, new QTableWidgetItem(createdDateTime.toString("MM-dd hh:mm")));
        
        // Store favorite_id and flight_id as user data
        QTableWidgetItem* item = ui->favoritesTableWidget->item(row, 0);
        item->setData(Qt::UserRole, favorite["favorite_id"].toInt());
        item->setData(Qt::UserRole + 1, favorite["flight_id"].toInt());
    }
    
    // Adjust column widths
    ui->favoritesTableWidget->resizeColumnsToContents();
}

void FavoritesWindow::updateUserSummary()
{
    QString summary = QString("当前用户：%1").arg(m_username.isEmpty() ? "-" : m_username);
    ui->userSummaryLabel->setText(summary);
}

int FavoritesWindow::currentSelectedFlightId() const
{
    int currentRow = ui->favoritesTableWidget->currentRow();
    if (currentRow >= 0 && currentRow < ui->favoritesTableWidget->rowCount()) {
        QTableWidgetItem* item = ui->favoritesTableWidget->item(currentRow, 0);
        if (item) {
            return item->data(Qt::UserRole + 1).toInt();
        }
    }
    return -1;
}

int FavoritesWindow::currentSelectedFavoriteId() const
{
    int currentRow = ui->favoritesTableWidget->currentRow();
    if (currentRow >= 0 && currentRow < ui->favoritesTableWidget->rowCount()) {
        QTableWidgetItem* item = ui->favoritesTableWidget->item(currentRow, 0);
        if (item) {
            return item->data(Qt::UserRole).toInt();
        }
    }
    return -1;
}
