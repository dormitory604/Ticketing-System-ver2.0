#include "booking_dialog.h"
#include "ui_booking_dialog.h"

#include <QRandomGenerator>
#include <QPainter>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QLocale>

BookingDialog::BookingDialog(int userId, int flightId, const QString &flightNumber,
                             double ticketPrice,
                             const QStringList &passengers, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::BookingDialog)
    , m_userId(userId)
    , m_flightId(flightId)
    , m_flightNumber(flightNumber)
    , m_paymentCompleted(false)
    , m_passengers(passengers)
    , m_ticketPrice(ticketPrice)
{
    ui->setupUi(this);
    ui->flightLabel->setText(tr("航班：%1").arg(flightNumber));
    updatePriceLabel();

    populatePassengers(passengers);

    ui->baggageComboBox->addItems({tr("15kg"), tr("20kg"), tr("25kg"), tr("30kg")});
    ui->paymentMethodComboBox->addItems({tr("支付宝"), tr("微信"), tr("银联"), tr("信用卡")});

    connect(ui->passengerComboBox, &QComboBox::editTextChanged, this, &BookingDialog::updateSubmitState);
    connect(ui->baggageComboBox, &QComboBox::currentTextChanged, this, &BookingDialog::updateSubmitState);
    connect(ui->paymentMethodComboBox, &QComboBox::currentTextChanged, this, &BookingDialog::updateSubmitState);
    connect(ui->cancelButton, &QPushButton::clicked, this, &BookingDialog::reject);

    regenerateQrCode();
    updateSubmitState();
}

BookingDialog::~BookingDialog()
{
    delete ui;
}

void BookingDialog::populatePassengers(const QStringList &passengers)
{
    ui->passengerComboBox->clear();
    ui->passengerComboBox->addItems(passengers);
    if (!passengers.isEmpty()) {
        ui->passengerComboBox->setCurrentIndex(0);
    }
}

void BookingDialog::on_generateQrButton_clicked()
{
    regenerateQrCode();
}

void BookingDialog::regenerateQrCode()
{
    ui->qrLabel->setPixmap(buildQrPixmap());
    m_paymentCompleted = false;
    ui->statusLabel->setText(tr("二维码已更新，请支付 %1。")
                                 .arg(QLocale().toCurrencyString(m_ticketPrice)));
    updateSubmitState();
}

QPixmap BookingDialog::buildQrPixmap(int cellSize, int gridCount) const
{
    QPixmap pixmap(cellSize * gridCount, cellSize * gridCount);
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);
    painter.setPen(Qt::NoPen);

    for (int y = 0; y < gridCount; ++y) {
        for (int x = 0; x < gridCount; ++x) {
            const bool fillCell = QRandomGenerator::global()->bounded(2) == 0;
            painter.setBrush(fillCell ? Qt::black : Qt::white);
            painter.drawRect(x * cellSize, y * cellSize, cellSize, cellSize);
        }
    }
    return pixmap;
}

void BookingDialog::on_paymentDoneButton_clicked()
{
    m_paymentCompleted = true;
    ui->statusLabel->setText(tr("已确认支付 %1，可提交订单")
                                 .arg(QLocale().toCurrencyString(m_ticketPrice)));
    updateSubmitState();
}

void BookingDialog::on_submitButton_clicked()
{
    if (!m_paymentCompleted) {
        ui->statusLabel->setText(tr("请先完成支付确认"));
        return;
    }

    const QString passenger = selectedPassengerName();
    const QString baggage = ui->baggageComboBox->currentText();
    const QString payment = ui->paymentMethodComboBox->currentText();

    emit bookingConfirmed(m_userId, m_flightId, passenger, baggage, payment);
    accept();
}

QString BookingDialog::selectedPassengerName() const
{
    const QString text = ui->passengerComboBox->currentText().trimmed();
    if (!text.isEmpty()) {
        return text;
    }
    return tr("默认乘客");
}

void BookingDialog::updateSubmitState()
{
    const bool validPassenger = !ui->passengerComboBox->currentText().trimmed().isEmpty();
    const bool validBaggage = !ui->baggageComboBox->currentText().isEmpty();
    const bool validPayment = !ui->paymentMethodComboBox->currentText().isEmpty();
    ui->submitButton->setEnabled(validPassenger && validBaggage && validPayment && m_paymentCompleted);
}

void BookingDialog::updatePriceLabel()
{
    const QString priceText = m_ticketPrice > 0.0
        ? QLocale().toCurrencyString(m_ticketPrice)
        : tr("待定");
    ui->priceLabel->setText(tr("票价：%1").arg(priceText));
}
