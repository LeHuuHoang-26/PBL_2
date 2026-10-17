#include "AppointmentController.h"
#include "libPBL2.h"

AppointmentController::AppointmentController() {}

LinkList<Appointment>* AppointmentController::getAllAppointments() {
    std::string path = libPBL2::getDataPath("Appointment/Appointment.txt");
    return Appointment::loadFromFile(path);
}

std::vector<Appointment> AppointmentController::searchAppointments(const std::string &keyword) {
    std::vector<Appointment> result;
    LinkList<Appointment> *list = getAllAppointments();
    if (!list) return result;

    auto *cur = list->getHeader()->next;
    while (cur != list->getFooter()) {
        const Appointment &a = cur->info;
        if (keyword.empty() || 
            a.getId().find(keyword) != std::string::npos || 
            a.getPatientId().find(keyword) != std::string::npos ||
            a.getDoctorId().find(keyword) != std::string::npos) {
            result.push_back(a);
        }
        cur = cur->next;
    }

    delete list;
    return result;
}

std::vector<Appointment> AppointmentController::filterByState(int state) {
    std::vector<Appointment> result;
    LinkList<Appointment> *list = getAllAppointments();
    if (!list) return result;

    auto *cur = list->getHeader()->next;
    while (cur != list->getFooter()) {
        if (cur->info.getState() == state) {
            result.push_back(cur->info);
        }
        cur = cur->next;
    }

    delete list;
    return result;
}

int AppointmentController::getTotalAppointmentCount() {
    LinkList<Appointment> *list = getAllAppointments();
    if (!list) return 0;
    int count = list->count;
    delete list;
    return count;
}
