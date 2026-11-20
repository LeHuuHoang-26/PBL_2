#ifndef APPCONSTANTS_H
#define APPCONSTANTS_H

namespace AppConstants {

constexpr int APPOINTMENT_GRACE_MINUTES = 20;
constexpr int APPOINTMENT_SLOT_MINUTES = 45;
constexpr int MIN_CALENDAR_YEAR = 2000;
constexpr int MAX_CALENDAR_YEAR = 2099;

namespace Paths {
    inline constexpr const char *PATIENT = "Data/Patient.txt";
    inline constexpr const char *MANAGER = "Data/Manager.txt";
    inline constexpr const char *DOCTOR = "Data/Doctor/Doctor.txt";
    inline constexpr const char *SPECIALIZATION = "Data/Doctor/Specialization.txt";
    inline constexpr const char *APPOINTMENT = "Data/Appointment.txt";
    inline constexpr const char *SYMPTOM = "Data/Symptom/Symptom.txt";
    inline constexpr const char *SYMPTOM_PATIENT = "Data/Symptom/SymptomPatient.txt";
    inline constexpr const char *SYMPTOM_PATIENT_DETAIL = "Data/Symptom/SymptomPatientDetail.txt";
    inline constexpr const char *PERMISSION_MANAGER = "Data/PermissionManager.txt";
    inline constexpr const char *PERMISSION_CATALOG = "Config/Permission.txt";
    inline constexpr const char *HOLIDAYS = "Config/Lichnghile.txt";
}

}

#endif
