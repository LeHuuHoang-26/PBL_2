#include "DoctorController.h"
#include "libPBL2.h"
#include <QMap>
#include <fstream>

static QMap<std::string, QList<QPair<QString, QString>>> s_specListCache;

static const QList<QPair<QString, QString>> &getSpecializationListRef(const std::string &specPath) {
    auto it = s_specListCache.constFind(specPath);
    if (it != s_specListCache.constEnd()) {
        return it.value();
    }

    QList<QPair<QString, QString>> result;
    std::ifstream file(specPath);
    if (file.is_open()) {
        std::string line;
        std::getline(file, line);
        std::getline(file, line);
        while (std::getline(file, line)) {
            auto separator = line.find('|');
            if (separator == std::string::npos) continue;
            std::string id = line.substr(0, separator);
            std::string label = line.substr(separator + 1);
            libPBL2::trim(id);
            libPBL2::trim(label);
            result.append(qMakePair(QString::fromUtf8(label.c_str()), QString::fromUtf8(id.c_str())));
        }
    }
    return s_specListCache.insert(specPath, result).value();
}

Doctor *DoctorController::findById(LinkList<Doctor> *doctors, const std::string &id) {
    return libPBL2::findByIdWithCursor(doctors, id, [](const Doctor &d) { return d.getID(); });
}

void DoctorController::sortDoctors(LinkList<Doctor> &list, DoctorSortBy criteria, bool ascending, const std::string &specPath) {
    libPBL2::qsort(list, [criteria, ascending, &specPath](const Doctor &a, const Doctor &b) {
        int cmp = 0;
        switch (criteria) {
            case DoctorSortBy::ID:
                cmp = libPBL2::compareNatural(a.getID(), b.getID());
                break;
            case DoctorSortBy::Name:
                cmp = libPBL2::compareVietnameseNames(a.getfullName(), b.getfullName());
                break;
            case DoctorSortBy::Rating:
                if (a.getRating() != b.getRating()) cmp = (a.getRating() < b.getRating()) ? -1 : 1;
                break;
            case DoctorSortBy::Fee:
                if (a.getFee() != b.getFee()) cmp = (a.getFee() < b.getFee()) ? -1 : 1;
                break;
            case DoctorSortBy::Specialty: {
                std::string specA = DoctorController::getSpecialty(a.getSpId(), specPath).toUtf8().toStdString();
                std::string specB = DoctorController::getSpecialty(b.getSpId(), specPath).toUtf8().toStdString();
                cmp = libPBL2::compareVietnameseStrings(specA, specB);
                break;
            }
            case DoctorSortBy::Room: {
                int roomA = libPBL2::extractRoomNumber(a.getRoomId());
                int roomB = libPBL2::extractRoomNumber(b.getRoomId());
                if (roomA != roomB) {
                    cmp = (roomA < roomB) ? -1 : 1;
                } else {
                    cmp = libPBL2::compareNatural(libPBL2::canonicalRoomKey(a.getRoomId()),
                                                  libPBL2::canonicalRoomKey(b.getRoomId()));
                }
                break;
            }
            case DoctorSortBy::State:
                if (a.getState() != b.getState()) cmp = (a.getState() > b.getState()) ? -1 : 1;
                break;
        }
        if (cmp == 0) cmp = libPBL2::compareNatural(a.getID(), b.getID());
        return ascending ? (cmp < 0) : (cmp > 0);
    });
}

void DoctorController::sortDoctors(LinkList<Doctor> *list, DoctorSortBy criteria, bool ascending, const std::string &specPath) {
    if (list) sortDoctors(*list, criteria, ascending, specPath);
}

std::string DoctorController::generateNextId(const LinkList<Doctor> *doctors) {
    return libPBL2::generateNextId(doctors, "D_", [](const Doctor &d) { return d.getID(); }, "D");
}

QString DoctorController::getSpecialty(const std::string &spId, const std::string &specPath) {
    if (spId.empty()) return QStringLiteral("Đa khoa");
    const auto &list = getSpecializationListRef(specPath);
    QString qSpId = QString::fromUtf8(spId.c_str());
    for (const auto &item : list) {
        if (item.second == qSpId) {
            return item.first;
        }
    }
    return QStringLiteral("Đa khoa");
}

QList<QPair<QString, QString>> DoctorController::getSpecializationList(const std::string &specPath) {
    return getSpecializationListRef(specPath);
}

void DoctorController::clearSpecialtyCache() {
    s_specListCache.clear();
}
