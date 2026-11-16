#include "PatientController.h"
#include "libPBL2.h"
#include <sstream>

Patient *PatientController::findById(LinkList<Patient> *patients, const std::string &id) {
    if (id.empty() || !patients || !patients->getHeader()) return nullptr;
    auto *node = patients->getHeader()->next;
    while (node && node->next) {
        if (!node->info.isDefault() && node->info.getID() == id) return &node->info;
        node = node->next;
    }
    return nullptr;
}

void PatientController::updateSymptomStatus(const std::string &patientId,
                                           LinkList<SymptomPatient> *spList,
                                           const LinkList<Appointment> *aList) {
    (void)patientId;
    (void)spList;
    (void)aList;
}
