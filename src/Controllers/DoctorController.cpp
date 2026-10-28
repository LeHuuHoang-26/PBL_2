#include "DoctorController.h"
#include "libPBL2.h"
#include <fstream>
#include <sstream>

Doctor *DoctorController::findById(LinkList<Doctor> *doctors, const std::string &id) {
    if (!doctors || !doctors->getHeader()) return nullptr;
    auto *node = doctors->getHeader()->next;
    while (node && node->next) {
        if (node->info.getID() == id) return &node->info;
        node = node->next;
    }
    return nullptr;
}

int DoctorController::count(const LinkList<Doctor> *doctors) {
    if (!doctors || !doctors->getHeader()) return 0;
    int c = 0;
    auto *n = doctors->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getValid()) c++;
        n = n->next;
    }
    return c;
}

std::string DoctorController::generateNextId(const LinkList<Doctor> *doctors) {
    int maxId = 0;
    if (doctors && doctors->getHeader()) {
        auto *n = doctors->getHeader()->next;
        while (n && n->next) {
            std::string id = n->info.getID();
            if (id.rfind("D_", 0) == 0) {
                try {
                    int val = std::stoi(id.substr(2));
                    if (val > maxId) maxId = val;
                } catch (...) {}
            }
            n = n->next;
        }
    }
    char buf[32];
    std::snprintf(buf, sizeof(buf), "D_%03d", maxId + 1);
    return std::string(buf);
}

static QList<QPair<QString, QString>> s_specCache;
static bool s_specCacheLoaded = false;

QList<QPair<QString, QString>> DoctorController::getSpecializationList(const std::string &specPath) {
    if (s_specCacheLoaded && !s_specCache.isEmpty()) {
        return s_specCache;
    }
    s_specCache.clear();
    std::ifstream file(specPath);
    if (!file.is_open()) return s_specCache;
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
        s_specCache.append(qMakePair(QString::fromUtf8(label.c_str()), QString::fromUtf8(id.c_str())));
    }
    s_specCacheLoaded = true;
    return s_specCache;
}

QString DoctorController::getSpecialty(const std::string &id, const std::string &specPath) {
    if (id.empty()) return QStringLiteral("Đa khoa");
    auto list = getSpecializationList(specPath);
    for (const auto &item : list) {
        if (item.second == QString::fromUtf8(id.c_str())) {
            return item.first;
        }
    }
    return QStringLiteral("Đa khoa");
}
