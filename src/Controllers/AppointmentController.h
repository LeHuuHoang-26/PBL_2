#ifndef APPOINTMENTCONTROLLER_H
#define APPOINTMENTCONTROLLER_H

#include "../Models/Appointment.h"
#include "../DataStructures/LinkList.h"
#include <QList>
#include <QDate>
#include <QString>
#include <string>

class Doctor;
class Patient;
class SymptomPatient;
class SymptomPatientDetail;

struct HolidayEntry {
    QString date;
    QString note;
};

struct RescheduledResultItem {
    QString oldApptId;
    QString newApptId;
    QString patientName;
    QString doctorName;
    QString oldDateTime;
    QString newDateTime;
};

class AppointmentController {
public:
    static int count(const LinkList<Appointment> *appointments);
    static QList<Appointment*> getAppointmentsForDate(const LinkList<Appointment> *appointments, const QDate &targetDate);
    static QList<Appointment*> getUpcomingAppointments(const LinkList<Appointment> *appointments, const QDate &today);
    static void sortAppointmentsByDate(QList<Appointment*> &list);
    static void getMonthDayCounts(const LinkList<Appointment> *appointments, int year, int month, int dayCounts[32]);
    static std::string generateNextId(const LinkList<Appointment> *appointments);
    static QDate getLunarNewYear(int year);
    static bool isTetHoliday(const QDate &date, int holidayDays = 5);
    static bool isHoliday(const QDate &date, const std::string &configPath);
    static bool isHolidayInList(const QDate &date, const QList<HolidayEntry> &list);
    static bool isSlotBooked(const LinkList<Appointment> *appointments, const std::string &doctorId, const Date &targetTime, const std::string &excludeApptId = "");
    static QString getHolidayNote(const QDate &date, const std::string &configPath);
    static QString getHolidayNoteFromList(const QDate &date, const QList<HolidayEntry> &list);
    static QList<HolidayEntry> loadHolidays(const std::string &configPath);
    static bool saveHolidays(const std::string &configPath, const QList<HolidayEntry> &list);
    static void clearHolidayCache();

    static QList<Appointment*> getAffectedAppointmentsForHoliday(
        const LinkList<Appointment> *appointments,
        const QList<HolidayEntry> &currentHolidays,
        const QList<HolidayEntry> &prospectiveHolidays
    );

    static int rescheduleAppointmentsLinear(
        LinkList<Appointment> *appointments,
        LinkList<SymptomPatient> *symptomPatients,
        LinkList<SymptomPatientDetail> *symptomPatientDetails,
        const LinkList<Doctor> *doctors,
        const LinkList<Patient> *patients,
        const QList<Appointment*> &affectedAppointments,
        const QList<HolidayEntry> &prospectiveHolidays,
        QString &outSummaryReport,
        QList<RescheduledResultItem> *outResultItems = nullptr
    );
};

#endif // APPOINTMENTCONTROLLER_H
