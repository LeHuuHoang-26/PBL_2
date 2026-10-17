#ifndef APPOINTMENTVIEW_H
#define APPOINTMENTVIEW_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <string>
#include "AppointmentController.h"

class AppointmentView : public QWidget {
    Q_OBJECT
private:
    AppointmentController controller; // Tầng Controller xử lý nghiệp vụ lịch hẹn
    QLineEdit *searchEdit;
    QPushButton *btnSearch;
    QPushButton *btnRefresh;
    QTableWidget *tableAppointments;
    QLabel *statusLabel;

public:
    explicit AppointmentView(QWidget *parent = nullptr);
    void loadData(const std::string &keyword = "");

private slots:
    void onSearchClicked();
    void onRefreshClicked();
};

#endif // APPOINTMENTVIEW_H
