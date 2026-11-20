#include "PatientController.h"
#include "libPBL2.h"

Patient *PatientController::findById(LinkList<Patient> *patients, const std::string &id) {
    if (id.empty() || !patients || !patients->getHeader()) return nullptr;
    auto *node = patients->getHeader()->next;
    while (node && node->next) {
        if (!node->info.isDefault() && node->info.getID() == id) return &node->info;
        node = node->next;
    }
    return nullptr;
}

void PatientController::sortPatients(LinkList<Patient> &list, PatientSortBy criteria, bool ascending) {
    libPBL2::qsort(list, [criteria, ascending](const Patient &a, const Patient &b) {
        int cmp = 0;
        switch (criteria) {
            case PatientSortBy::ID:
                cmp = libPBL2::compareNatural(a.getID(), b.getID());
                break;
            case PatientSortBy::Name:
                cmp = libPBL2::compareVietnameseNames(a.getfullName(), b.getfullName());
                break;
            case PatientSortBy::Age:
                if (a.getAge() != b.getAge()) cmp = (a.getAge() < b.getAge()) ? -1 : 1;
                break;
            case PatientSortBy::CCCD:
                cmp = libPBL2::compareNatural(a.getCCCD(), b.getCCCD());
                break;
            case PatientSortBy::Phone:
                cmp = libPBL2::compareNatural(a.getPhone(), b.getPhone());
                break;
            case PatientSortBy::Gender:
                if (a.getGender() != b.getGender()) cmp = a.getGender() ? -1 : 1;
                break;
        }
        if (cmp == 0) cmp = libPBL2::compareNatural(a.getID(), b.getID());
        return ascending ? (cmp < 0) : (cmp > 0);
    });
}

void PatientController::sortPatients(LinkList<Patient> *list, PatientSortBy criteria, bool ascending) {
    if (list) sortPatients(*list, criteria, ascending);
}
