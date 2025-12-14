 #include "flightdialog.h"
#include "ui_flightdialog.h"

FlightDialog::FlightDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FlightDialog)
{
    ui->setupUi(this);

    // 设置日期时间编辑框默认为当前时间
    ui->dtpTime->setDateTime(QDateTime::currentDateTime());
    // 设置到达时间默认往后推2小时
    ui->dtpArrivalTime->setDateTime(QDateTime::currentDateTime().addSecs(7200));
}

FlightDialog::~FlightDialog()
{
    delete ui;
}

// 编辑模式：把旧数据填入输入框
void FlightDialog::setFlightData(int id, const QString &no, const QString &model,
                                 const QString &origin, const QString &dest,
                                 const QDateTime &deptTime, const QDateTime &arrTime,
                                 double price, int totalSeats, int remainingSeats)
{
    m_flightId = id;  // 记住ID，更新时需要用
    m_remainingSeats = remainingSeats;  // 记住当前的余票
    m_oldTotalSeats = totalSeats;  // 记住原本的总座位数
    ui->txtFlightNum->setText(no);
    ui->txtModel->setText(model);
    ui->txtOrigin->setText(origin);
    ui->txtDest->setText(dest);
    ui->dtpTime->setDateTime(deptTime);
    ui->dtpArrivalTime->setDateTime(arrTime);
    ui->spinPrice->setValue(price);
    ui->spinSeats->setValue(totalSeats);

    this->setWindowTitle("修改航班");
}

// 获取用户输入的数据打包成 JSON
QJsonObject FlightDialog::getFlightData() const
{
    QJsonObject obj;
    // 如果 m_flightId 是 -1，说明是添加，不需要传 ID (或者传 0，看后端需求)
    // 如果 m_flightId > 0，说明是更新，必须传 ID
    if(m_flightId != -1)
    {
        obj["flight_id"] = m_flightId;
    }

    obj["flight_number"] = ui->txtFlightNum->text();
    obj["model"] = ui->txtModel->text();
    obj["origin"] = ui->txtOrigin->text();
    obj["destination"] = ui->txtDest->text();
    obj["departure_time"] = ui->dtpTime->dateTime().toString("yyyy-MM-dd HH:mm:ss");
    obj["arrival_time"] = ui->dtpArrivalTime->dateTime().toString("yyyy-MM-dd HH:mm:ss");
    obj["price"] = ui->spinPrice->value();

    // 获取新的总座位数
    int newTotalSeats = ui->spinSeats->value();
    obj["total_seats"] = newTotalSeats;

    // 如果是添加模式(id=-1)，Server会自动把余票设为总票数
    // 如果是修改模式，需要计算：新余票 = 旧余票 + (新总座 - 旧总座)
    if (m_flightId != -1)
    {
        int diff = newTotalSeats - m_oldTotalSeats;
        int newRemaining = m_remainingSeats + diff;

        // 防止出现负数
        if (newRemaining < 0) newRemaining = 0;

        obj["remaining_seats"] = newRemaining;
    }
    else
    {
        // 添加模式
        obj["remaining_seats"] = newTotalSeats;
    }

    return obj;
}
