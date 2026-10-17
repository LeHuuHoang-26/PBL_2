#include "DoctorController.h"
#include "libPBL2.h"

DoctorController::DoctorController() {}

LinkList<Doctor>* DoctorController::getAllDoctors() {
    std::string path = libPBL2::getDataPath("Doctor/Doctor.txt");
    return Doctor::loadFromFile(path);
}

std::vector<Doctor> DoctorController::searchDoctors(const std::string &keyword) {
    std::vector<Doctor> result;
    LinkList<Doctor> *list = getAllDoctors();
    if (!list) return result;

    auto *cur = list->getHeader()->next;
    while (cur != list->getFooter()) {
        const Doctor &d = cur->info;
        if (keyword.empty() || 
            d.getfullName().find(keyword) != std::string::npos || 
            d.getID().find(keyword) != std::string::npos) {
            result.push_back(d);
        }
        cur = cur->next;
    }

    delete list;
    return result;
}

bool DoctorController::getDoctorById(const std::string &id, Doctor &outDoctor) {
    LinkList<Doctor> *list = getAllDoctors();
    if (!list) return false;

    bool found = false;
    auto *cur = list->getHeader()->next;
    while (cur != list->getFooter()) {
        if (cur->info.getID() == id) {
            outDoctor = cur->info;
            found = true;
            break;
        }
        cur = cur->next;
    }

    delete list;
    return found;
}

int DoctorController::getTotalDoctorCount() {
    LinkList<Doctor> *list = getAllDoctors();
    if (!list) return 0;
    int count = list->count;
    delete list;
    return count;
}
