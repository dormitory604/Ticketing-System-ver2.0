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
    void setFlightData(int id, const QString &no, const QString &origin,
                       const QString &dest, const QDateTime &time,
                       const QDateTime &arrTime,
                       double price, int seats);

    // 用户点确定后，调用这个函数获取填好的数据
    QJsonObject getFlightData() const;

private:
    Ui::FlightDialog *ui;
    int m_flightId = -1; // 存储航班ID，如果是添加模式则为-1
};

#endif // FLIGHTDIALOG_H
