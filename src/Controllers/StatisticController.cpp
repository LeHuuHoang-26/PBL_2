#include "StatisticController.h"
#include "Doctor.h"
#include "Patient.h"
#include "Appointment.h"
#include "LinkList.h"
#include "libPBL2.h"

StatisticController::StatisticController() {}

ClinicStatistics StatisticController::getClinicStatistics() {
    ClinicStatistics stats = {0, 0, 0, 0, 0, 0};

    // 1. Đếm bác sĩ
    LinkList<Doctor> *doctors = Doctor::loadFromFile(libPBL2::getDataPath("Doctor/Doctor.txt"));
    if (doctors) {
        stats.totalDoctors = doctors->count;
        delete doctors;
    }

    // 2. Đếm bệnh nhân
    LinkList<Patient> *patients = Patient::loadFromFile(libPBL2::getDataPath("Patient.txt"));
    if (patients) {
        stats.totalPatients = patients->count;
        delete patients;
    }

    // 3. Đếm và phân loại lịch hẹn
    LinkList<Appointment> *apps = Appointment::loadFromFile(libPBL2::getDataPath("Appointment/Appointment.txt"));
    if (apps) {
        stats.totalAppointments = apps->count;
        auto *cur = apps->getHeader()->next;
        while (cur != apps->getFooter()) {
            int state = cur->info.getState();
            if (state == 1) stats.completedAppointments++;
            else if (state == 2) stats.canceledAppointments++;
            else stats.pendingAppointments++;
            cur = cur->next;
        }
        delete apps;
    }

    return stats;
}
