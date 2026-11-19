#ifndef LIBPBL2_H
#define LIBPBL2_H

#include <string>
#include <cstdio>
#include "../DataStructures/LinkList.h"
#include "Date.h"
#include "AppExceptions.h"

class Patient;
class Doctor;
class Appointment;
class Manager;
class PermissionManager;

namespace libPBL2 {

    enum class PatientSortBy {
        ID,
        Name,
        Age,
        CCCD,
        Phone,
        Gender
    };

    enum class DoctorSortBy {
        ID,
        Name,
        Rating,
        Fee,
        Specialty,
        Room,
        State
    };

    enum class AppointmentSortBy {
        ID
    };

    enum class AccountSortBy {
        ID,
        Username,
        Name,
        Role,
        State
    };

    inline void trim(std::string &str);
    inline size_t utf8Length(const std::string &str);
    inline std::string padRightUtf8(const std::string &str, size_t targetWidth);

    inline int safeParseInt(const std::string &str, const std::string &fieldName, int defaultVal = 0);
    inline double safeParseDouble(const std::string &str, const std::string &fieldName, double defaultVal = 0.0);

    inline void safeAtomicCommit(const std::string &tempPath, const std::string &finalPath);

    template <typename T, typename IdGetter>
    std::string generateNextId(const LinkList<T> *list, const std::string &prefix, IdGetter getId, const std::string &altPrefix = "");

    inline int compareNatural(const std::string &a, const std::string &b);
    inline std::string extractLastName(const std::string &fullName);
    inline int compareVietnameseNames(const std::string &name1, const std::string &name2);
    inline int compareDates(const Date &da, const Date &db);
    inline int extractRoomNumber(const std::string &roomStr);

    template <typename T>
    void swapVal(T &a, T &b);

    template <typename T, typename Compare>
    typename LinkList<T>::Node* partitionNodes(typename LinkList<T>::Node *left,
                                                typename LinkList<T>::Node *right,
                                                Compare comp);

    template <typename T, typename Compare>
    void qsortNodes(typename LinkList<T>::Node *left, typename LinkList<T>::Node *right, Compare comp);

    template <typename T, typename Compare>
    void qsort(LinkList<T> &list, Compare comp);

    template <typename T, typename Compare>
    void qsort(LinkList<T> *list, Compare comp);

    template <typename T, typename Compare>
    void qsort(T *arr, int low, int high, Compare comp);

    template <typename Container, typename Compare>
    void qsortRandomAccess(Container &c, int low, int high, Compare comp);

    template <typename Container, typename Compare>
    void qsort(Container &c, Compare comp);

    inline void sortPatients(LinkList<Patient> &list, PatientSortBy criteria = PatientSortBy::Name, bool ascending = true);
    inline void sortPatients(LinkList<Patient> *list, PatientSortBy criteria = PatientSortBy::Name, bool ascending = true);

    inline void sortDoctors(LinkList<Doctor> &list, DoctorSortBy criteria = DoctorSortBy::Name, bool ascending = true);
    inline void sortDoctors(LinkList<Doctor> *list, DoctorSortBy criteria = DoctorSortBy::Name, bool ascending = true);

    inline void sortAppointments(LinkList<Appointment> &list, AppointmentSortBy criteria = AppointmentSortBy::ID, bool ascending = true);
    inline void sortAppointments(LinkList<Appointment> *list, AppointmentSortBy criteria = AppointmentSortBy::ID, bool ascending = true);

    inline void sortManagers(LinkList<Manager> &list, AccountSortBy criteria = AccountSortBy::ID, bool ascending = true, const LinkList<PermissionManager> *permissionRecords = nullptr);
    inline void sortManagers(LinkList<Manager> *list, AccountSortBy criteria = AccountSortBy::ID, bool ascending = true, const LinkList<PermissionManager> *permissionRecords = nullptr);

}

#include "libPBL2.cpp"

#endif
