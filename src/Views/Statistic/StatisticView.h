#ifndef STATISTICVIEW_H
#define STATISTICVIEW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include "StatisticController.h"

class StatisticView : public QWidget {
    Q_OBJECT
private:
    StatisticController controller; // Tầng Controller thống kê số liệu
    QLabel *lblDoctorCount;
    QLabel *lblPatientCount;
    QLabel *lblAppointmentCount;
    QLabel *lblCompletedCount;

public:
    explicit StatisticView(QWidget *parent = nullptr);
    void refreshStatistics();

private slots:
    void onRefreshClicked();
};

#endif // STATISTICVIEW_H
