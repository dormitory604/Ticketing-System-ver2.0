#ifndef FAVORITES_WINDOW_H
#define FAVORITES_WINDOW_H

#include <QMainWindow>
#include <QJsonArray>
#include <QJsonObject>

namespace Ui {
class FavoritesWindow;
}

class FavoritesWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FavoritesWindow(QWidget *parent = nullptr);
    ~FavoritesWindow();

    void setUserId(int userId);
    void setUsername(const QString& username);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void on_backButton_clicked();
    void on_refreshButton_clicked();
    void on_removeFavoriteButton_clicked();
    void on_favoritesTableWidget_itemSelectionChanged();

    void handleMyFavoritesResult(const QJsonArray& favorites);
    void handleRemoveFavoriteSuccess(const QString& message);
    void handleRemoveFavoriteFailed(const QString& message);
    void handleGeneralError(const QString& message);

private:
    void populateFavoritesTable(const QJsonArray& favorites);
    void updateUserSummary();
    int currentSelectedFlightId() const;
    int currentSelectedFavoriteId() const;

    Ui::FavoritesWindow *ui;
    int m_userId;
    QString m_username;
    QJsonArray m_latestFavorites;
};

#endif // FAVORITES_WINDOW_H
