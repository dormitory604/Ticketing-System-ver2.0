#ifndef MYORDERS_WINDOW_H
#define MYORDERS_WINDOW_H

#include <QDialog>
#include <QJsonArray>
#include <QShowEvent>

namespace Ui {
class MyOrdersWindow;
}

class MyOrdersWindow : public QDialog
{
    Q_OBJECT

public:
    explicit MyOrdersWindow(QWidget *parent = nullptr);
    ~MyOrdersWindow();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void on_refreshButton_clicked();
    void on_cancelOrderButton_clicked();

    void handleOrdersResult(const QJsonArray& orders);
    void handleCancelSuccess(const QString& message);
    void handleCancelFailed(const QString& message);
    void handleGeneralError(const QString& message);

private:
    void requestOrders();
    int currentBookingId() const;
    void populateOrdersTable(const QJsonArray& orders);

    Ui::MyOrdersWindow *ui;
    QJsonArray m_latestOrders;
};

#endif // MYORDERS_WINDOW_H
