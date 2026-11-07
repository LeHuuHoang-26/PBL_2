#ifndef APPOINTMENTVIEW_H
#define APPOINTMENTVIEW_H

#include <QWidget>
#include <QDate>
#include <QList>
#include <string>

class MainWindow;
class QCalendarWidget;
class QTableWidget;
class QLabel;
class QPushButton;
class Appointment;

class AppointmentView : public QObject {
    Q_OBJECT

public:
    explicit AppointmentView(MainWindow *mainWindow, QObject *parent = nullptr);

    QWidget *createDashboardWidget(QWidget *parent = nullptr);
    QWidget *createBookingWidget(QWidget *parent = nullptr);
    QWidget *createAppointmentListWidget(QWidget *parent = nullptr);
    QWidget *createHolidayWidget(QWidget *parent = nullptr);

    bool showBookingDialog(const QDate &defaultDate = QDate::currentDate());
    void showHolidayManagementDialog(QWidget *parent = nullptr);
    void showAppointmentDetailDialog(Appointment *appointment);
    void editAppointmentDialog(const std::string &id);
    void highlightCalendarAppointments(QCalendarWidget *cal, int year, int month);
    QList<Appointment*> getAppointmentsForDate(const QDate &targetDate) const;
    void reapplyCurrentSort();

private:
    MainWindow *mainWindow = nullptr;
    int listSortColumn = -1;
    bool listSortAscending = true;
    int listCurrentPage = 0;
};

#endif // APPOINTMENTVIEW_H
