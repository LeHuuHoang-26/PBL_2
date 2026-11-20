#ifndef PATIENTCONTROLLER_H
#define PATIENTCONTROLLER_H

#include "Patient.h"
#include "LinkList.h"
#include <string>

enum class PatientSortBy {
    ID,
    Name,
    Age,
    CCCD,
    Phone,
    Gender
};

class PatientController {
public:
    static Patient *findById(LinkList<Patient> *patients, const std::string &id);
    static void sortPatients(LinkList<Patient> &list, PatientSortBy criteria = PatientSortBy::Name, bool ascending = true);
    static void sortPatients(LinkList<Patient> *list, PatientSortBy criteria = PatientSortBy::Name, bool ascending = true);
};

#endif
