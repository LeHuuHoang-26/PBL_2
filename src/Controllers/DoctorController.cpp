#include "DoctorController.h"
#include "libPBL2.h"
#include <QMap>
#include <fstream>

static QMap<std::string, QList<QPair<QString, QString>>> s_specListCache;

Doctor *DoctorController::findById(LinkList<Doctor> *doctors, const std::string &id) {
    if (id.empty() || !doctors || !doctors->getHeader()) return nullptr;
    auto *node = doctors->getHeader()->next;
    while (node && node->next) {
        if (!node->info.isDefault() && node->info.getID() == id) return &node->info;
        node = node->next;
    }
    return nullptr;
}

std::string DoctorController::generateNextId(const LinkList<Doctor> *doctors) {
    return libPBL2::generateNextId(doctors, "D_", [](const Doctor &d) { return d.getID(); }, "D");
}

QString DoctorController::getSpecialty(const std::string &spId, const std::string &specPath) {
    if (spId.empty()) return QStringLiteral("Đa khoa");
    auto list = getSpecializationList(specPath);
    for (const auto &item : list) {
        if (item.second == QString::fromUtf8(spId.c_str())) {
            return item.first;
        }
    }
    return QStringLiteral("Đa khoa");
}

QList<QPair<QString, QString>> DoctorController::getSpecializationList(const std::string &specPath) {
    if (s_specListCache.contains(specPath)) {
        return s_specListCache[specPath];
    }

    QList<QPair<QString, QString>> result;
    std::ifstream file(specPath);
    if (!file.is_open()) return result;
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
    s_specListCache[specPath] = result;
    return result;
}

void DoctorController::clearSpecialtyCache() {
    s_specListCache.clear();
}
