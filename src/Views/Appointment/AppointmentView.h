#ifndef APPOINTMENTVIEW_H
#define APPOINTMENTVIEW_H

#include <QWidget>
#include <QDate>
#include <QList>
#include <memory>
#include <string>
#include "AppointmentController.h"
#include "AppConstants.h"

class MainWindow;
class QCalendarWidget;

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

    bool showAppointmentDetailDialog(const std::string &appointmentId);
    void showAppointmentDetailDialog(Appointment *appointment);
    void editAppointmentDialog(const std::string &id);
    void showRescheduledSummaryDialog(
        QWidget *parent,
        const QList<RescheduledResultItem> &items,
        int totalAffected,
        const QString &customTitle = ""
    );

    void showHolidayManagementDialog(QWidget *parent = nullptr);

    void resetToFirstPage();

private:
    MainWindow *mainWindow = nullptr;
    int listSortColumn = -1;
    bool listSortAscending = false;
    std::weak_ptr<int> listCurrentPagePtr;

    bool processHolidayAdditionWithReschedule(
        QWidget *parentDialog,
        const QList<HolidayEntry> &curList,
        const QList<HolidayEntry> &prospectiveList,
        const std::string &configPath = AppConstants::Paths::HOLIDAYS
    );
};

#endif
