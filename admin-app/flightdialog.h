#ifndef FLIGHTDIALOG_H
#define FLIGHTDIALOG_H

#include <QDialog>
#include <QJsonObject>

namespace Ui {
class FlightDialog;
}

class FlightDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FlightDialog(QWidget *parent = nullptr);
    ~FlightDialog();

    //如果是编辑模式，调用这个函数把旧数据填进去
    void setFlightData(int id, const QString &no, const QString &model,
                       const QString &origin, const QString &dest,
                       const QDateTime &deptTime, const QDateTime &arrTime,
                       double price, int totalSeats, int remainingSeats);

    // 用户点确定后，调用这个函数获取填好的数据
    QJsonObject getFlightData() const;

private:
    Ui::FlightDialog *ui;
    int m_flightId = -1; // 存储航班ID，如果是添加模式则为-1
    int m_remainingSeats = 0;  // 暂存余票
    int m_oldTotalSeats = 0;  // 暂存原本的总座位数，用来计算差值
};

#endif // FLIGHTDIALOG_H
