#ifndef APPOINTMENTCONTROLLER_H
#define APPOINTMENTCONTROLLER_H

#include "../Models/Appointment.h"
#include "../DataStructures/LinkList.h"
#include <QList>
#include <QDate>
#include <QDateTime>
#include <QString>
#include <string>

class Doctor;
class Patient;
class Symptom;
class SymptomPatient;
class SymptomPatientDetail;

struct HolidayEntry {
    QString date;
    QString note;
};

struct RescheduledResultItem {
    QString patientName;
    QString doctorName;
    QString oldDoctorName;
    QString oldDateTime;
    QString newDateTime;
};

class AppointmentController {
public:
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
    static bool isPatientSlotBooked(
        const LinkList<Appointment> *appointments,
        const LinkList<SymptomPatient> *symptomPatients,
        const std::string &patientId,
        const Date &targetTime,
        const std::string &excludeApptId = ""
    );
    static void validateBooking(
        const LinkList<Appointment> *appointments,
        const LinkList<SymptomPatient> *symptomPatients,
        const std::string &patientId,
        const std::string &doctorId,
        const QDateTime &targetDateTime,
        const std::string &excludeApptId = "",
        bool checkHoliday = false,
        const std::string &configPath = "",
        const LinkList<Doctor> *doctors = nullptr,
        const LinkList<Patient> *patients = nullptr
    );
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

    static QList<Appointment*> getAffectedAppointmentsForDoctor(
        const LinkList<Appointment> *appointments,
        const std::string &doctorId
    );

    static int reassignAppointmentsForDoctor(
        LinkList<Appointment> *appointments,
        LinkList<SymptomPatient> *symptomPatients,
        LinkList<SymptomPatientDetail> *symptomPatientDetails,
        const LinkList<Symptom> *symptoms,
        const LinkList<Doctor> *doctors,
        const LinkList<Patient> *patients,
        const std::string &oldDoctorId,
        const QList<HolidayEntry> &holidays,
        QList<RescheduledResultItem> *outResultItems = nullptr,
        int *outFailedCount = nullptr,
        QList<std::string> *outInsertedApptIds = nullptr,
        QList<std::string> *outInsertedSpIds = nullptr,
        QList<Appointment*> *outModifiedOldAppts = nullptr
    );

    static int rescheduleAppointmentsLinear(
        LinkList<Appointment> *appointments,
        LinkList<SymptomPatient> *symptomPatients,
        LinkList<SymptomPatientDetail> *symptomPatientDetails,
        const LinkList<Doctor> *doctors,
        const LinkList<Patient> *patients,
        const QList<Appointment*> &affectedAppointments,
        const QList<HolidayEntry> &prospectiveHolidays,
        QList<RescheduledResultItem> *outResultItems = nullptr,
        const LinkList<Symptom> *symptoms = nullptr,
        QList<std::string> *outInsertedApptIds = nullptr,
        QList<std::string> *outInsertedSpIds = nullptr,
        QList<Appointment*> *outModifiedOldAppts = nullptr
    );

    static void rollbackInsertedAppointments(
        LinkList<Appointment> *appointments,
        LinkList<SymptomPatient> *symptomPatients,
        LinkList<SymptomPatientDetail> *symptomPatientDetails,
        const QList<std::string> &insertedApptIds,
        const QList<std::string> &insertedSpIds,
        const QList<Appointment*> &modifiedOldAppts
    );

    static int syncExpiredAppointments(LinkList<Appointment> *appointments);
};

#endif
