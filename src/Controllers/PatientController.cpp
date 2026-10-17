#include "PatientController.h"
#include "libPBL2.h"

PatientController::PatientController() {}

LinkList<Patient>* PatientController::getAllPatients() {
    std::string path = libPBL2::getDataPath("Patient.txt");
    return Patient::loadFromFile(path);
}

std::vector<Patient> PatientController::searchPatients(const std::string &keyword) {
    std::vector<Patient> result;
    LinkList<Patient> *list = getAllPatients();
    if (!list) return result;

    auto *cur = list->getHeader()->next;
    while (cur != list->getFooter()) {
        const Patient &p = cur->info;
        if (keyword.empty() || 
            p.getfullName().find(keyword) != std::string::npos || 
            p.getID().find(keyword) != std::string::npos ||
            p.getCCCD().find(keyword) != std::string::npos ||
            p.getPhone().find(keyword) != std::string::npos) {
            result.push_back(p);
        }
        cur = cur->next;
    }

    delete list;
    return result;
}

Queue<QueuePatient>* PatientController::getPatientQueue() {
    std::string path = libPBL2::getDataPath("QueuePatient.txt");
    return QueuePatient::loadFromFile(path);
}

int PatientController::getTotalPatientCount() {
    LinkList<Patient> *list = getAllPatients();
    if (!list) return 0;
    int count = list->count;
    delete list;
    return count;
}
