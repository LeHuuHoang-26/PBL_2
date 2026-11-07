#include "PatientController.h"
#include "libPBL2.h"
#include <sstream>

Patient *PatientController::findById(LinkList<Patient> *patients, const std::string &id) {
    if (!patients || !patients->getHeader()) return nullptr;
    auto *node = patients->getHeader()->next;
    while (node && node->next) {
        if (node->info.getID() == id) return &node->info;
        node = node->next;
    }
    return nullptr;
}

int PatientController::count(const LinkList<Patient> *patients) {
    return patients ? patients->count : 0;
}

QStringList PatientController::getSymptomsForPatient(const std::string &patientId,
                                                    const LinkList<SymptomPatient> *spList,
                                                    const LinkList<SymptomPatientDetail> *spdList,
                                                    const LinkList<Symptom> *sList) {
    QStringList result;
    if (!spList || !spdList || !sList) return result;
    auto *spNode = spList->getHeader()->next;
    while (spNode && spNode->next) {
        if (spNode->info.getP_id() == patientId) {
            std::string spId = spNode->info.getSP_id();
            auto *spdNode = spdList->getHeader()->next;
            while (spdNode && spdNode->next) {
                if (spdNode->info.getSP_id() == spId) {
                    std::string sId = spdNode->info.getS_id();
                    auto *sNode = sList->getHeader()->next;
                    while (sNode && sNode->next) {
                        if (sNode->info.getS_id() == sId) {
                            result.append(QString::fromUtf8(sNode->info.getSName().c_str()));
                            break;
                        }
                        sNode = sNode->next;
                    }
                }
                spdNode = spdNode->next;
            }
        }
        spNode = spNode->next;
    }
    result.removeDuplicates();
    return result;
}

QString PatientController::getUnresolvedSymptoms(const std::string &patientId,
                                                 const LinkList<SymptomPatient> *spList,
                                                 const LinkList<SymptomPatientDetail> *spdList,
                                                 const LinkList<Symptom> *sList,
                                                 const LinkList<Appointment> *aList) {
    QStringList names;
    if (!spList || !spList->getHeader()) return QStringLiteral("Không có triệu chứng");
    bool hasAnyRecord = false;
    auto *record = spList->getHeader()->next;
    while (record && record->next) {
        if (record->info.getP_id() == patientId) {
            hasAnyRecord = true;
            bool isResolved = false;
            if (aList && aList->getHeader()) {
                auto *aNode = aList->getHeader()->next;
                while (aNode && aNode->next) {
                    if (aNode->info.getSymptomPatientId() == record->info.getSP_id() && aNode->info.getState() == 1) {
                        isResolved = true;
                        break;
                    }
                    aNode = aNode->next;
                }
            }
            if (isResolved) {
                record = record->next;
                continue;
            }
            if (spdList && spdList->getHeader()) {
                std::string spId = record->info.getSP_id();
                auto *spdNode = spdList->getHeader()->next;
                while (spdNode && spdNode->next) {
                    if (spdNode->info.getSP_id() == spId) {
                        std::string sId = spdNode->info.getS_id();
                        if (sList && sList->getHeader()) {
                            auto *s = sList->getHeader()->next;
                            while (s && s->next) {
                                if (s->info.getS_id() == sId) {
                                    names.append(QString::fromUtf8(s->info.getSName().c_str()));
                                    break;
                                }
                                s = s->next;
                            }
                        }
                    }
                    spdNode = spdNode->next;
                }
            }
        }
        record = record->next;
    }
    names.removeDuplicates();
    if (!hasAnyRecord) return QStringLiteral("Không có triệu chứng");
    return names.isEmpty() ? QStringLiteral("Đã khám hết") : QStringLiteral("Chưa khám: %1").arg(names.join(QStringLiteral(", ")));
}

void PatientController::updateSymptomStatus(const std::string &patientId,
                                           LinkList<SymptomPatient> *spList,
                                           const LinkList<Appointment> *aList) {
    (void)patientId;
    (void)spList;
    (void)aList;
}
