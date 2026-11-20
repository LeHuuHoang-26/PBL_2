#ifndef STATISTICCONTROLLER_H
#define STATISTICCONTROLLER_H

#include "Patient.h"
#include "Doctor.h"
#include "Appointment.h"
#include "SymptomPatient.h"
#include "LinkList.h"
#include <string>

struct DoctorLeaderboardItem {
    std::string doctorId;
    std::string doctorName;
    std::string specialtyId;
    std::string roomId;
    double rating = 0.0;
    int appointmentCount = 0;
};

struct SpecialtyDemandItem {
    std::string spId;
    int count = 0;
};

struct TimeSeriesItem {
    std::string label;
    int day = 0;
    int month = 0;
    int year = 0;
    double revenue = 0.0;
    int completedCount = 0;
    int delayedCount = 0;
    int rescheduledCount = 0;
};

class StatisticController {
public:
    static int patientCount(const LinkList<Patient> *items);
    static int doctorCount(const LinkList<Doctor> *items);
    static int appointmentCount(const LinkList<Appointment> *items);
    static int pendingAppointmentCount(const LinkList<Appointment> *items);

    static void appointmentStatusCounts(
        const LinkList<Appointment> *appts,
        int &completed, int &pending, int &delayed, int &rescheduled,
        int currentDay, int currentMonth, int currentYear, int currentHour, int currentMinute,
        int currentSecond = 0,
        int startDay = 0, int startMonth = 0, int startYear = 0,
        int endDay = 0, int endMonth = 0, int endYear = 0
    );

    static void patientGenderCounts(const LinkList<Patient> *patients, int &male, int &female);
    static void patientAgeDistribution(const LinkList<Patient> *patients, int &under18, int &from18to45, int &from46to60, int &over60);

    static void patientDemographicsInRange(
        const LinkList<Patient> *patients,
        const LinkList<Appointment> *appts,
        const LinkList<SymptomPatient> *symptomPatients,
        int startDay, int startMonth, int startYear,
        int endDay, int endMonth, int endYear,
        int &male, int &female,
        int &under18, int &from18to45, int &from46to60, int &over60
    );

    static int getTopDoctors(
        const LinkList<Appointment> *appts,
        const LinkList<Doctor> *docs,
        DoctorLeaderboardItem outList[],
        int maxCount,
        int startDay = 0, int startMonth = 0, int startYear = 0,
        int endDay = 0, int endMonth = 0, int endYear = 0
    );

    static int getSpecialtyDemand(
        const LinkList<Appointment> *appts,
        const LinkList<Doctor> *docs,
        SpecialtyDemandItem outList[],
        int maxCount,
        int startDay = 0, int startMonth = 0, int startYear = 0,
        int endDay = 0, int endMonth = 0, int endYear = 0
    );

    static int getTimeSeriesDaily(
        const LinkList<Appointment> *appts,
        const LinkList<Doctor> *docs,
        int startDay, int startMonth, int startYear,
        int endDay, int endMonth, int endYear,
        int curDay, int curMonth, int curYear, int curHour, int curMin,
        TimeSeriesItem outItems[], int maxItems,
        int curSec = 0
    );

    static int getTimeSeriesMonthlyRange(
        const LinkList<Appointment> *appts,
        const LinkList<Doctor> *docs,
        int startDay, int startMonth, int startYear,
        int endDay, int endMonth, int endYear,
        int curDay, int curMonth, int curYear, int curHour, int curMin,
        TimeSeriesItem outItems[], int maxItems,
        int curSec = 0
    );
};

#endif
