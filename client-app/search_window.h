#ifndef SEARCH_WINDOW_H
#define SEARCH_WINDOW_H

#include <QMainWindow>
#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>

class MyOrdersWindow;
class ProfileWindow;
class BookingDialog;
class FavoritesWindow;

namespace Ui {
class SearchWindow;
}

class SearchWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit SearchWindow(QWidget *parent = nullptr);
    ~SearchWindow();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void on_searchButton_clicked();
    void on_bookButton_clicked();
    void on_myOrdersButton_clicked();
    void on_myFavoritesButton_clicked();
    void on_profileButton_clicked();
    void on_addToFavoritesButton_clicked();

    void handleSearchResults(const QJsonArray& flights);
    void handleBookingSuccess(const QJsonObject& bookingData);
    void handleBookingFailed(const QString& message);
    void handleGeneralError(const QString& message);
    void handleAddFavoriteSuccess(const QString& message);
    void handleAddFavoriteFailed(const QString& message);

private:
    void populateFlightsTable(const QJsonArray& flights);
    int currentSelectedFlightId() const;
    QJsonObject currentSelectedFlight() const;
    void updateUserSummary();
    void setupFilterControls();
    void updateFilterHint();
    QString currentCabinClassCode() const;
    QStringList selectedPassengerTypes() const;
    QString passengerSummaryText() const;
    QStringList passengerCandidates() const;

    Ui::SearchWindow *ui;
    QJsonArray m_latestFlights;
    MyOrdersWindow *m_ordersWindow;
    ProfileWindow *m_profileWindow;
    FavoritesWindow *m_favoritesWindow;
};

#endif // SEARCH_WINDOW_H
