#ifndef PATIENTCONTROLLER_H
#define PATIENTCONTROLLER_H

#include "../Models/Patient.h"
#include "../Models/Symptom.h"
#include "../Models/SymptomPatient.h"
#include "../Models/SymptomPatientDetail.h"
#include "../Models/Appointment.h"
#include "../DataStructures/LinkList.h"
#include <string>
#include <QString>
#include <QStringList>

class PatientController {
public:
    static Patient *findById(LinkList<Patient> *patients, const std::string &id);
};

#endif // PATIENTCONTROLLER_H
