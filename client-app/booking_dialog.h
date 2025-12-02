#ifndef BOOKING_DIALOG_H
#define BOOKING_DIALOG_H

#include <QDialog>
#include <QStringList>
#include <QPixmap>

namespace Ui {
class BookingDialog;
}

class BookingDialog : public QDialog
{
    Q_OBJECT
public:
    BookingDialog(int userId, int flightId, const QString& flightNumber,
                  double ticketPrice,
                  const QStringList& passengers, QWidget *parent = nullptr);
    ~BookingDialog();

signals:
    void bookingConfirmed(int userId, int flightId, const QString& passengerName,
                          const QString& baggageOption, const QString& paymentMethod);

private slots:
    void on_generateQrButton_clicked();
    void on_paymentDoneButton_clicked();
    void on_submitButton_clicked();

private:
    void populatePassengers(const QStringList& passengers);
    void regenerateQrCode();
    void updateSubmitState();
    QString selectedPassengerName() const;
    QPixmap buildQrPixmap(int cellSize = 8, int gridCount = 25) const;
    void updatePriceLabel();

    Ui::BookingDialog *ui;
    int m_userId;
    int m_flightId;
    QString m_flightNumber;
    bool m_paymentCompleted;
    QStringList m_passengers;
    double m_ticketPrice;
};

#endif // BOOKING_DIALOG_H
