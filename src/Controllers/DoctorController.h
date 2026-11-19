#ifndef DOCTORCONTROLLER_H
#define DOCTORCONTROLLER_H

#include "../Models/Doctor.h"
#include "../DataStructures/LinkList.h"
#include <string>
#include <QString>
#include <QList>
#include <QPair>

class DoctorController {
public:
    static Doctor *findById(LinkList<Doctor> *doctors, const std::string &id);
    static std::string generateNextId(const LinkList<Doctor> *doctors);
    static QString getSpecialty(const std::string &spId, const std::string &specPath);
    static QList<QPair<QString, QString>> getSpecializationList(const std::string &specPath);
    static void clearSpecialtyCache();
};

#endif
