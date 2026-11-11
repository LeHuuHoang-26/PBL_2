#ifndef LIBPBL2_H
#define LIBPBL2_H

#include <string>
#include "../DataStructures/LinkList.h"
#include "../Models/Permission.h"
#include "Date.h"

class Patient;
class Doctor;
class Appointment;
class Manager;

namespace libPBL2 {

    using Permission = ::Permission;
    using PermissionType = ::PermissionType;

    // String utilities
    inline void trim(std::string &str);
    inline void filterNewline(std::string &str);
    inline size_t utf8Length(const std::string &str);
    inline std::string padRightUtf8(const std::string &str, size_t targetWidth);

    // Permission checking
    inline bool isAllowed(const Manager &M, PermissionType Per, const LinkList<Permission> *PermissionList = nullptr);

    // -------------------------------------------------------------
    // Generic QuickSort Algorithm (Khai báo template)
    // -------------------------------------------------------------

    template <typename T>
    void swapVal(T &a, T &b);

    // Partition logic for doubly linked list
    template <typename T, typename Compare>
    typename LinkList<T>::Node* partitionNodes(typename LinkList<T>::Node *left,
                                                typename LinkList<T>::Node *right,
                                                Compare comp);

    // Recursive node quicksort
    template <typename T, typename Compare>
    void qsortNodes(typename LinkList<T>::Node *left, typename LinkList<T>::Node *right, Compare comp);

    // QuickSort on LinkList<T>
    template <typename T, typename Compare>
    void qsort(LinkList<T> &list, Compare comp);

    template <typename T, typename Compare>
    void qsort(LinkList<T> *list, Compare comp);

    // QuickSort on C-arrays
    template <typename T, typename Compare>
    void qsort(T *arr, int low, int high, Compare comp);

    // QuickSort on random-access indexable containers (QList, std::vector, etc.)
    template <typename Container, typename Compare>
    void qsortRandomAccess(Container &c, int low, int high, Compare comp);

    template <typename Container, typename Compare>
    void qsort(Container &c, Compare comp);

    // Comparison helpers
    inline int compareNatural(const std::string &a, const std::string &b);
    inline std::string extractLastName(const std::string &fullName);
    inline int compareVietnameseNames(const std::string &name1, const std::string &name2);
    inline int compareDates(const Date &da, const Date &db);
    inline int extractRoomNumber(const std::string &roomStr);

    // -------------------------------------------------------------
    // Patient Sorting
    // -------------------------------------------------------------
    enum class PatientSortBy {
        ID,
        Name,
        Age,
        CCCD,
        Phone,
        Gender
    };
    inline void sortPatients(LinkList<Patient> &list, PatientSortBy criteria = PatientSortBy::Name, bool ascending = true);
    inline void sortPatients(LinkList<Patient> *list, PatientSortBy criteria = PatientSortBy::Name, bool ascending = true);

    // -------------------------------------------------------------
    // Doctor Sorting
    // -------------------------------------------------------------
    enum class DoctorSortBy {
        ID,
        Name,
        Rating,
        Fee,
        Specialty,
        Room,
        State
    };
    inline void sortDoctors(LinkList<Doctor> &list, DoctorSortBy criteria = DoctorSortBy::Name, bool ascending = true);
    inline void sortDoctors(LinkList<Doctor> *list, DoctorSortBy criteria = DoctorSortBy::Name, bool ascending = true);

    // -------------------------------------------------------------
    // Appointment Sorting
    // -------------------------------------------------------------
    enum class AppointmentSortBy {
        Date,
        ID,
        State,
        DoctorID
    };
    inline void sortAppointments(LinkList<Appointment> &list, AppointmentSortBy criteria = AppointmentSortBy::Date, bool ascending = true);
    inline void sortAppointments(LinkList<Appointment> *list, AppointmentSortBy criteria = AppointmentSortBy::Date, bool ascending = true);

    // -------------------------------------------------------------
    // Account (Manager) Sorting
    // -------------------------------------------------------------
    enum class AccountSortBy {
        ID,
        Username,
        Name,
        Role,
        State
    };
    inline void sortManagers(LinkList<Manager> &list, AccountSortBy criteria = AccountSortBy::ID, bool ascending = true);
    inline void sortManagers(LinkList<Manager> *list, AccountSortBy criteria = AccountSortBy::ID, bool ascending = true);

} // namespace libPBL2

#include "libPBL2.cpp"

#endif // LIBPBL2_H
