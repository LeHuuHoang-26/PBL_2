#ifndef DOCTORCONTROLLER_H
#define DOCTORCONTROLLER_H

#include "Doctor.h"
#include "LinkList.h"
#include "AppConstants.h"
#include <string>
#include <QString>
#include <QList>
#include <QPair>

enum class DoctorSortBy {
    ID,
    Name,
    Rating,
    Fee,
    Specialty,
    Room,
    State
};

class DoctorController {
public:
    static Doctor *findById(LinkList<Doctor> *doctors, const std::string &id);
    static void sortDoctors(LinkList<Doctor> &list, DoctorSortBy criteria = DoctorSortBy::Name, bool ascending = true, const std::string &specPath = AppConstants::Paths::SPECIALIZATION);
    static void sortDoctors(LinkList<Doctor> *list, DoctorSortBy criteria = DoctorSortBy::Name, bool ascending = true, const std::string &specPath = AppConstants::Paths::SPECIALIZATION);
    static std::string generateNextId(const LinkList<Doctor> *doctors);
    static QString getSpecialty(const std::string &spId, const std::string &specPath = AppConstants::Paths::SPECIALIZATION);
    static QList<QPair<QString, QString>> getSpecializationList(const std::string &specPath = AppConstants::Paths::SPECIALIZATION);
    static void clearSpecialtyCache();
};

#endif
