#ifndef LIBPBL2_CPP
#define LIBPBL2_CPP

#include "libPBL2.h"
#include "../Models/Manager.h"
#include "../Models/Patient.h"
#include "../Models/Doctor.h"
#include "../Models/Appointment.h"
#include "../Controllers/ManagerController.h"

#include <cctype>
#include <stdexcept>
#include <QString>
#include <QCollator>
#include <QLocale>

namespace libPBL2 {

inline void trim(std::string &str) {
    size_t head = str.find_first_not_of(" \t\r\n");
    if (head == std::string::npos) {
        str.clear();
        return;
    }
    size_t tail = str.find_last_not_of(" \t\r\n");
    str = str.substr(head, tail - head + 1);
}

inline size_t utf8Length(const std::string &str) {
    size_t count = 0;
    for (unsigned char byte : str) {
        if ((byte & 0xC0) != 0x80) count++;
    }
    return count;
}

inline std::string padRightUtf8(const std::string &str, size_t targetWidth) {
    size_t ulen = utf8Length(str);
    if (ulen >= targetWidth) return str;
    return str + std::string(targetWidth - ulen, ' ');
}

inline int safeParseInt(const std::string &str, const std::string &fieldName, int defaultVal) {
    std::string text = str;
    trim(text);
    if (text.empty()) return defaultVal;
    try {
        size_t processed = 0;
        int parsed = std::stoi(text, &processed);
        if (processed != text.length()) {
            throw DataFormatException("Trường [" + fieldName + "] chứa ký tự không hợp lệ: '" + str + "'");
        }
        return parsed;
    } catch (const std::invalid_argument &) {
        throw DataFormatException("Trường [" + fieldName + "] không đúng định dạng số nguyên: '" + str + "'");
    } catch (const std::out_of_range &) {
        throw DataFormatException("Trường [" + fieldName + "] vượt quá giới hạn số nguyên: '" + str + "'");
    }
}

inline double safeParseDouble(const std::string &str, const std::string &fieldName, double defaultVal) {
    std::string text = str;
    trim(text);
    if (text.empty()) return defaultVal;
    try {
        size_t processed = 0;
        double parsed = std::stod(text, &processed);
        if (processed != text.length()) {
            throw DataFormatException("Trường [" + fieldName + "] chứa ký tự không hợp lệ: '" + str + "'");
        }
        return parsed;
    } catch (const std::invalid_argument &) {
        throw DataFormatException("Trường [" + fieldName + "] không đúng định dạng số thực: '" + str + "'");
    } catch (const std::out_of_range &) {
        throw DataFormatException("Trường [" + fieldName + "] vượt quá giới hạn số thực: '" + str + "'");
    }
}

inline void safeAtomicCommit(const std::string &tempPath, const std::string &finalPath) {
    const std::string backupPath = finalPath + ".bak";
    std::remove(backupPath.c_str());
    bool backedUp = (std::rename(finalPath.c_str(), backupPath.c_str()) == 0);
    if (std::rename(tempPath.c_str(), finalPath.c_str()) != 0) {
        if (backedUp) {
            std::rename(backupPath.c_str(), finalPath.c_str());
        }
        std::remove(tempPath.c_str());
        throw FileIOException(finalPath, "Không thể cập nhật tệp dữ liệu sau khi ghi tạm.");
    }
    if (backedUp) {
        std::remove(backupPath.c_str());
    }
}

template <typename T, typename IdGetter>
std::string generateNextId(const LinkList<T> *list, const std::string &prefix, IdGetter getId, const std::string &altPrefix) {
    int highest = 0;
    if (list && list->getHeader()) {
        auto *node = list->getHeader()->next;
        while (node && node->next) {
            if (!node->info.isDefault()) {
                std::string currentId = getId(node->info);
                size_t skip = 0;
                if (currentId.rfind(prefix, 0) == 0 && currentId.length() > prefix.length()) {
                    skip = prefix.length();
                } else if (!altPrefix.empty() && currentId.rfind(altPrefix, 0) == 0 && currentId.length() > altPrefix.length()) {
                    skip = altPrefix.length();
                }
                if (skip > 0) {
                    try {
                        int num = std::stoi(currentId.substr(skip));
                        if (num > highest) highest = num;
                    } catch (const std::invalid_argument &) {
                    } catch (const std::out_of_range &) {
                    }
                }
            }
            node = node->next;
        }
    }
    char buffer[48];
    std::snprintf(buffer, sizeof(buffer), "%s%03d", prefix.c_str(), highest + 1);
    return std::string(buffer);
}

inline int compareNatural(const std::string &a, const std::string &b) {
    size_t ia = 0, ib = 0;
    const size_t szA = a.size(), szB = b.size();
    while (ia < szA && ib < szB) {
        unsigned char ca = static_cast<unsigned char>(a[ia]);
        unsigned char cb = static_cast<unsigned char>(b[ib]);
        if (std::isdigit(ca) && std::isdigit(cb)) {
            size_t begA = ia;
            while (ia < szA && std::isdigit(static_cast<unsigned char>(a[ia]))) ia++;
            size_t begB = ib;
            while (ib < szB && std::isdigit(static_cast<unsigned char>(b[ib]))) ib++;

            size_t nonZeroA = begA;
            while (nonZeroA < ia && a[nonZeroA] == '0') nonZeroA++;
            size_t nonZeroB = begB;
            while (nonZeroB < ib && b[nonZeroB] == '0') nonZeroB++;

            size_t spanA = ia - nonZeroA;
            size_t spanB = ib - nonZeroB;
            if (spanA != spanB) return (spanA < spanB) ? -1 : 1;
            for (size_t k = 0; k < spanA; ++k) {
                if (a[nonZeroA + k] != b[nonZeroB + k]) {
                    return (a[nonZeroA + k] < b[nonZeroB + k]) ? -1 : 1;
                }
            }
        } else {
            unsigned char la = static_cast<unsigned char>(std::tolower(ca));
            unsigned char lb = static_cast<unsigned char>(std::tolower(cb));
            if (la != lb) return (la < lb) ? -1 : 1;
            ia++;
            ib++;
        }
    }
    if (ia < szA || ib < szB) return (ia < szA) ? 1 : -1;
    if (szA != szB) return (szA < szB) ? -1 : 1;
    return 0;
}

inline std::string extractLastName(const std::string &fullName) {
    size_t right = fullName.find_last_not_of(" \t\r\n");
    if (right == std::string::npos) return "";
    size_t left = fullName.find_last_of(" \t", right);
    if (left == std::string::npos) return fullName.substr(0, right + 1);
    return fullName.substr(left + 1, right - left);
}

inline int compareVietnameseNames(const std::string &name1, const std::string &name2) {
    static const QCollator viCollator = []() {
        QCollator c(QLocale(QLocale::Vietnamese));
        c.setCaseSensitivity(Qt::CaseInsensitive);
        return c;
    }();
    std::string l1 = extractLastName(name1);
    std::string l2 = extractLastName(name2);
    int res = viCollator.compare(QString::fromUtf8(l1.c_str()), QString::fromUtf8(l2.c_str()));
    if (res != 0) return (res < 0) ? -1 : 1;
    res = viCollator.compare(QString::fromUtf8(name1.c_str()), QString::fromUtf8(name2.c_str()));
    if (res != 0) return (res < 0) ? -1 : 1;
    return 0;
}

inline int compareDates(const Date &da, const Date &db) {
    if (da.getYear() != db.getYear()) return (da.getYear() < db.getYear()) ? -1 : 1;
    if (da.getMonth() != db.getMonth()) return (da.getMonth() < db.getMonth()) ? -1 : 1;
    if (da.getDay() != db.getDay()) return (da.getDay() < db.getDay()) ? -1 : 1;
    if (da.getHour() != db.getHour()) return (da.getHour() < db.getHour()) ? -1 : 1;
    if (da.getMinute() != db.getMinute()) return (da.getMinute() < db.getMinute()) ? -1 : 1;
    return 0;
}

inline int extractRoomNumber(const std::string &roomStr) {
    int value = 0;
    bool hasDigits = false;
    for (char ch : roomStr) {
        if (std::isdigit(static_cast<unsigned char>(ch))) {
            value = value * 10 + (ch - '0');
            hasDigits = true;
        } else if (hasDigits) {
            break;
        }
    }
    return hasDigits ? value : -1;
}

template <typename T>
void swapVal(T &a, T &b) {
    if (&a == &b) return;
    T hold = static_cast<T&&>(a);
    a = static_cast<T&&>(b);
    b = static_cast<T&&>(hold);
}

template <typename T, typename Compare>
typename LinkList<T>::Node* partitionNodes(typename LinkList<T>::Node *left,
                                            typename LinkList<T>::Node *right,
                                            Compare comp) {
    typename LinkList<T>::Node *lead = left;
    typename LinkList<T>::Node *runner = left;
    while (runner != right && runner->next != right && runner->next != nullptr) {
        lead = lead->next;
        runner = runner->next->next;
    }
    if (lead != right) {
        swapVal(lead->info, right->info);
    }

    T pivotVal = right->info;
    typename LinkList<T>::Node *idx = left->prev;

    for (typename LinkList<T>::Node *it = left; it != right; it = it->next) {
        if (comp(it->info, pivotVal)) {
            idx = (idx == left->prev) ? left : idx->next;
            if (idx != it) {
                swapVal(idx->info, it->info);
            }
        }
    }
    idx = (idx == left->prev) ? left : idx->next;
    if (idx != right) {
        swapVal(idx->info, right->info);
    }
    return idx;
}

template <typename T, typename Compare>
void qsortNodes(typename LinkList<T>::Node *left, typename LinkList<T>::Node *right, Compare comp) {
    if (!left || !right || left == right || right->next == left) return;
    typename LinkList<T>::Node *bound = partitionNodes<T, Compare>(left, right, comp);
    if (bound != left && bound->prev) {
        qsortNodes<T, Compare>(left, bound->prev, comp);
    }
    if (bound != right && bound->next) {
        qsortNodes<T, Compare>(bound->next, right, comp);
    }
}

template <typename T, typename Compare>
void qsort(LinkList<T> &list, Compare comp) {
    if (list.getCount() <= 1 || !list.getHeader() || !list.getFooter()) return;
    typename LinkList<T>::Node *headNode = list.getHeader()->next;
    typename LinkList<T>::Node *tailNode = list.getFooter()->prev;
    if (!headNode || !tailNode || headNode == list.getFooter() || headNode == tailNode) return;
    qsortNodes<T, Compare>(headNode, tailNode, comp);
}

template <typename T, typename Compare>
void qsort(LinkList<T> *list, Compare comp) {
    if (list) qsort<T, Compare>(*list, comp);
}

template <typename T, typename Compare>
void qsort(T *arr, int low, int high, Compare comp) {
    if (!arr || low >= high) return;
    int center = low + (high - low) / 2;
    if (center != high) swapVal(arr[center], arr[high]);
    T pivot = arr[high];
    int i = low - 1;
    for (int j = low; j < high; ++j) {
        if (comp(arr[j], pivot)) {
            i++;
            if (i != j) swapVal(arr[i], arr[j]);
        }
    }
    if (i + 1 != high) swapVal(arr[i + 1], arr[high]);
    int sep = i + 1;
    qsort(arr, low, sep - 1, comp);
    qsort(arr, sep + 1, high, comp);
}

template <typename Container, typename Compare>
void qsortRandomAccess(Container &c, int low, int high, Compare comp) {
    if (low >= high) return;
    int center = low + (high - low) / 2;
    if (center != high) swapVal(c[center], c[high]);
    auto pivot = c[high];
    int i = low - 1;
    for (int j = low; j < high; ++j) {
        if (comp(c[j], pivot)) {
            i++;
            if (i != j) swapVal(c[i], c[j]);
        }
    }
    if (i + 1 != high) swapVal(c[i + 1], c[high]);
    int sep = i + 1;
    qsortRandomAccess(c, low, sep - 1, comp);
    qsortRandomAccess(c, sep + 1, high, comp);
}

template <typename Container, typename Compare>
void qsort(Container &c, Compare comp) {
    if (c.size() <= 1) return;
    qsortRandomAccess(c, 0, static_cast<int>(c.size()) - 1, comp);
}

inline void sortPatients(LinkList<Patient> &list, PatientSortBy criteria, bool ascending) {
    qsort(list, [criteria, ascending](const Patient &a, const Patient &b) {
        int cmp = 0;
        switch (criteria) {
            case PatientSortBy::ID:
                cmp = compareNatural(a.getID(), b.getID());
                break;
            case PatientSortBy::Name:
                cmp = compareVietnameseNames(a.getfullName(), b.getfullName());
                break;
            case PatientSortBy::Age:
                if (a.getAge() != b.getAge()) cmp = (a.getAge() < b.getAge()) ? -1 : 1;
                break;
            case PatientSortBy::CCCD:
                cmp = compareNatural(a.getCCCD(), b.getCCCD());
                break;
            case PatientSortBy::Phone:
                cmp = compareNatural(a.getPhone(), b.getPhone());
                break;
            case PatientSortBy::Gender:
                if (a.getGender() != b.getGender()) cmp = a.getGender() ? -1 : 1;
                break;
        }
        if (cmp == 0) cmp = compareNatural(a.getID(), b.getID());
        return ascending ? (cmp < 0) : (cmp > 0);
    });
}

inline void sortPatients(LinkList<Patient> *list, PatientSortBy criteria, bool ascending) {
    if (list) sortPatients(*list, criteria, ascending);
}

inline void sortDoctors(LinkList<Doctor> &list, DoctorSortBy criteria, bool ascending) {
    qsort(list, [criteria, ascending](const Doctor &a, const Doctor &b) {
        int cmp = 0;
        switch (criteria) {
            case DoctorSortBy::ID:
                cmp = compareNatural(a.getID(), b.getID());
                break;
            case DoctorSortBy::Name:
                cmp = compareVietnameseNames(a.getfullName(), b.getfullName());
                break;
            case DoctorSortBy::Rating:
                if (a.getRating() != b.getRating()) cmp = (a.getRating() < b.getRating()) ? -1 : 1;
                break;
            case DoctorSortBy::Fee:
                if (a.getFee() != b.getFee()) cmp = (a.getFee() < b.getFee()) ? -1 : 1;
                break;
            case DoctorSortBy::Specialty:
                cmp = compareNatural(a.getSpId(), b.getSpId());
                break;
            case DoctorSortBy::Room: {
                int roomA = extractRoomNumber(a.getRoomId());
                int roomB = extractRoomNumber(b.getRoomId());
                if (roomA != roomB) {
                    cmp = (roomA < roomB) ? -1 : 1;
                } else {
                    cmp = compareNatural(a.getRoomId(), b.getRoomId());
                }
                break;
            }
            case DoctorSortBy::State:
                if (a.getState() != b.getState()) cmp = (a.getState() > b.getState()) ? -1 : 1;
                break;
        }
        if (cmp == 0) cmp = compareNatural(a.getID(), b.getID());
        return ascending ? (cmp < 0) : (cmp > 0);
    });
}

inline void sortDoctors(LinkList<Doctor> *list, DoctorSortBy criteria, bool ascending) {
    if (list) sortDoctors(*list, criteria, ascending);
}

inline void sortAppointments(LinkList<Appointment> &list, AppointmentSortBy criteria, bool ascending) {
    (void)criteria;
    qsort(list, [ascending](const Appointment &a, const Appointment &b) {
        int cmp = compareNatural(a.getId(), b.getId());
        if (cmp == 0) cmp = compareDates(a.getDateGet(), b.getDateGet());
        return ascending ? (cmp < 0) : (cmp > 0);
    });
}

inline void sortAppointments(LinkList<Appointment> *list, AppointmentSortBy criteria, bool ascending) {
    if (list) sortAppointments(*list, criteria, ascending);
}

inline void sortManagers(LinkList<Manager> &list, AccountSortBy criteria, bool ascending, const LinkList<PermissionManager> *permissionRecords) {
    qsort(list, [criteria, ascending, permissionRecords](const Manager &a, const Manager &b) {
        int cmp = 0;
        switch (criteria) {
            case AccountSortBy::ID:
                cmp = compareNatural(a.getID(), b.getID());
                break;
            case AccountSortBy::Username:
                cmp = compareNatural(a.getUsername(), b.getUsername());
                break;
            case AccountSortBy::Name:
                cmp = compareVietnameseNames(a.getfullName(), b.getfullName());
                break;
            case AccountSortBy::Role: {
                int ra = static_cast<int>(ManagerController::getEffectiveRole(&a, permissionRecords));
                int rb = static_cast<int>(ManagerController::getEffectiveRole(&b, permissionRecords));
                if (ra != rb) cmp = (ra < rb) ? -1 : 1;
                break;
            }
            case AccountSortBy::State:
                if (a.getState() != b.getState()) cmp = (a.getState() < b.getState()) ? -1 : 1;
                break;
        }
        if (cmp == 0) cmp = compareNatural(a.getID(), b.getID());
        return ascending ? (cmp < 0) : (cmp > 0);
    });
}

inline void sortManagers(LinkList<Manager> *list, AccountSortBy criteria, bool ascending, const LinkList<PermissionManager> *permissionRecords) {
    if (list) sortManagers(*list, criteria, ascending, permissionRecords);
}

}

#endif
