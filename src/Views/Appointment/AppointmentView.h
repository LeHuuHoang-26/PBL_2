#ifndef APPOINTMENTVIEW_H
#define APPOINTMENTVIEW_H

#include <QWidget>
#include <QDate>
#include <QList>
#include <string>
#include "../../Controllers/AppointmentController.h"

class MainWindow;
class QCalendarWidget;
class Appointment;

class AppointmentView : public QObject {
    Q_OBJECT

public:
    explicit AppointmentView(MainWindow *mainWindow, QObject *parent = nullptr);

    QList<Appointment*> getAppointmentsForDate(const QDate &targetDate) const;
    void highlightCalendarAppointments(QCalendarWidget *cal, int year, int month);

    bool showBookingDialog(const QDate &defaultDate = QDate::currentDate(), const std::string &preselectedPatientId = "");

    QWidget *createDashboardWidget(QWidget *parent = nullptr);
    QWidget *createBookingWidget(QWidget *parent = nullptr);
    QWidget *createAppointmentListWidget(QWidget *parent = nullptr);

    void showAppointmentDetailDialog(Appointment *appointment);
    void editAppointmentDialog(const std::string &id);
    void showRescheduledSummaryDialog(
        QWidget *parent,
        const QList<RescheduledResultItem> &items,
        int totalAffected,
        const QString &customTitle = ""
    );

    void showHolidayManagementDialog(QWidget *parent = nullptr);

    void reapplyCurrentSort();
    void resetToFirstPage();

private:
    MainWindow *mainWindow = nullptr;
    int listSortColumn = -1;
    bool listSortAscending = false;

    bool processHolidayAdditionWithReschedule(
        QWidget *parentDialog,
        const QList<HolidayEntry> &curList,
        const QList<HolidayEntry> &prospectiveList,
        const std::string &configPath = "Config/Lichnghile.txt"
    );
};

#endif
