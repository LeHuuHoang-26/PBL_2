#include "PermissionManager.h"
#include "libPBL2.h"
#include "../Utils/AppExceptions.h"
#include <fstream>
#include <sstream>
#include <QDateTime>
#include <QString>

PermissionManager::PermissionManager()
    : MA_id(""), Per_id(""), Expires_At("") {}

PermissionManager::PermissionManager(std::string maId, std::string perId, std::string expiresAt)
    : MA_id(maId), Per_id(perId), Expires_At(expiresAt) {}

PermissionManager::PermissionManager(const PermissionManager &other)
    : MA_id(other.MA_id), Per_id(other.Per_id), Expires_At(other.Expires_At) {}

std::string PermissionManager::getMA_id() const { return MA_id; }
std::string PermissionManager::getPer_id() const { return Per_id; }
std::string PermissionManager::getExpiresAt() const { return Expires_At; }

void PermissionManager::setMA_id(const std::string &maId) { MA_id = maId; }
void PermissionManager::setPer_id(const std::string &perId) { Per_id = perId; }
void PermissionManager::setExpiresAt(const std::string &expiresAt) { Expires_At = expiresAt; }

bool PermissionManager::isDefault() const {
    return MA_id.empty() && Per_id.empty();
}

void PermissionManager::setDefault() {
    MA_id.clear();
    Per_id.clear();
    Expires_At.clear();
}

bool PermissionManager::isTemporary() const {
    return !Expires_At.empty();
}

bool PermissionManager::isExpired() const {
    if (Expires_At.empty()) return false;
    auto qStr = QString::fromUtf8(Expires_At.c_str());
    auto parsed = QDateTime::fromString(qStr, Qt::ISODate);
    if (!parsed.isValid()) {
        parsed = QDateTime::fromString(qStr, "yyyy-MM-dd HH:mm:ss");
    }
    if (!parsed.isValid()) return true;
    return parsed <= QDateTime::currentDateTime();
}

std::string PermissionManager::toString() const {
    return MA_id + " | " + Per_id + " | " + Expires_At;
}

PermissionManager PermissionManager::loadFromString(const std::string &str) {
    if (str.empty()) return PermissionManager();
    std::stringstream ss(str);
    std::string maId, perId, expiry;
    if (!std::getline(ss, maId, '|') || !std::getline(ss, perId, '|')) {
        return PermissionManager();
    }
    std::getline(ss, expiry);
    libPBL2::trim(maId);
    libPBL2::trim(perId);
    libPBL2::trim(expiry);

    if (maId.empty() || perId.empty() || maId == "MA_ID" || maId.find("---") != std::string::npos) {
        return PermissionManager();
    }

    return PermissionManager(maId, perId, expiry);
}

LinkList<PermissionManager>* PermissionManager::loadFromFile(const std::string &path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để đọc dữ liệu phân quyền.");
    }

    LinkList<PermissionManager> *list = nullptr;
    try {
        list = new LinkList<PermissionManager>;
        std::string line;
        if (std::getline(in, line)) {
            if (line.find("MA_ID") != std::string::npos) {
                std::getline(in, line);
            } else {
                libPBL2::trim(line);
                if (!line.empty()) {
                    auto pm = loadFromString(line);
                    if (!pm.isDefault()) list->insert(pm);
                }
            }
        }

        while (std::getline(in, line)) {
            libPBL2::trim(line);
            if (line.empty()) continue;
            auto pm = loadFromString(line);
            if (!pm.isDefault()) {
                list->insert(pm);
            }
        }

        in.close();
        return list;
    } catch (...) {
        delete list;
        in.close();
        throw;
    }
}

bool PermissionManager::saveToFile(const std::string &path, const LinkList<PermissionManager> *list) {
    if (!list || !list->getHeader()) return false;
    const std::string tempPath = path + ".tmp";
    std::ofstream out(tempPath);
    if (!out.is_open()) {
        throw FileIOException(path, "Không thể mở tệp tạm để ghi dữ liệu phân quyền.");
    }

    try {
        out << "MA_ID | Per_ID | Expires_At\n";
        out << "-------+--------+--------------------\n";
        for (auto *curr = list->getHeader()->next; curr && curr->next; curr = curr->next) {
            if (!curr->info.isDefault()) {
                out << curr->info.toString() << '\n';
            }
        }
        out.flush();
        const bool writeOk = out.good();
        out.close();
        if (!writeOk || out.fail()) {
            std::remove(tempPath.c_str());
            throw FileIOException(path, "Lỗi ghi dữ liệu phân quyền xuống tệp tạm.");
        }
        libPBL2::safeAtomicCommit(tempPath, path);
        return true;
    } catch (...) {
        out.close();
        std::remove(tempPath.c_str());
        throw;
    }
}
