#include "flightdialog.h"
#include "ui_flightdialog.h"

FlightDialog::FlightDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FlightDialog)
{
    ui->setupUi(this);

    // 设置日期时间编辑框默认为当前时间
    ui->dtpTime->setDateTime(QDateTime::currentDateTime());
}

FlightDialog::~FlightDialog()
{
    delete ui;
}

// 编辑模式：把旧数据填入输入框
void FlightDialog::setFlightData(int id, const QString &no, const QString &origin,
                                 const QString &dest, const QDateTime &time,
                                 double price, int seats)
{
    m_flightId = id; // 记住ID，更新时需要用
    ui->txtFlightNum->setText(no);
    ui->txtOrigin->setText(origin);
    ui->txtDest->setText(dest);
    ui->dtpTime->setDateTime(time);
    ui->spinPrice->setValue(price);
    ui->spinSeats->setValue(seats);

    this->setWindowTitle("修改航班");
}

// 获取用户输入的数据打包成 JSON
QJsonObject FlightDialog::getFlightData() const
{
    QJsonObject obj;
    // 如果 m_flightId 是 -1，说明是添加，不需要传 ID (或者传 0，看后端需求)
    // 如果 m_flightId > 0，说明是更新，必须传 ID
    if(m_flightId != -1) {
        obj["flight_id"] = m_flightId;
    }

    obj["flight_number"] = ui->txtFlightNum->text();
    obj["origin"] = ui->txtOrigin->text();
    obj["destination"] = ui->txtDest->text();
    obj["departure_time"] = ui->dtpTime->dateTime().toString("yyyy-MM-dd HH:mm:ss");
    obj["price"] = ui->spinPrice->value();
    obj["remaining_seats"] = ui->spinSeats->value();

    return obj;
}
