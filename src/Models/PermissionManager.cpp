#include "PermissionManager.h"
#include "libPBL2.h"
#include "AppExceptions.h"
#include <ctime>
#include <fstream>
#include <sstream>

PermissionManager::PermissionManager()
    : MA_id(""), Per_id(""), Expires_At("") {}

PermissionManager::PermissionManager(std::string maId, std::string perId, std::string expiresAt)
    : MA_id(maId), Per_id(perId), Expires_At(expiresAt) {}

PermissionManager::PermissionManager(const PermissionManager &other)
    : MA_id(other.MA_id), Per_id(other.Per_id), Expires_At(other.Expires_At) {}

std::string PermissionManager::getMA_id() const { return MA_id; }
std::string PermissionManager::getPer_id() const { return Per_id; }
std::string PermissionManager::getExpiresAt() const { return Expires_At; }

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
    int y = 0, m = 0, d = 0, hh = 0, mm = 0, ss = 0;
    char sep1 = '\0', sep2 = '\0', dtSep = '\0', t1 = '\0', t2 = '\0';
    std::istringstream iss(Expires_At);
    if (!(iss >> y >> sep1 >> m >> sep2 >> d) || sep1 != '-' || sep2 != '-') {
        return true;
    }
    if (y < 1900 || y > 9999 || m < 1 || m > 12 || d < 1) {
        return true;
    }
    static const int kDaysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maxDay = kDaysInMonth[m];
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))) {
        maxDay = 29;
    }
    if (d > maxDay) return true;

    if (iss.peek() == 'T' || iss.peek() == ' ') {
        iss.get(dtSep);
        if (!(iss >> hh >> t1 >> mm) || t1 != ':') return true;
        if (iss.peek() == ':') {
            iss.get(t2);
            if (!(iss >> ss)) return true;
        }
    }
    if (hh < 0 || hh > 23 || mm < 0 || mm > 59 || ss < 0 || ss > 59) {
        return true;
    }

    std::tm tmVal{};
    tmVal.tm_year = y - 1900;
    tmVal.tm_mon = m - 1;
    tmVal.tm_mday = d;
    tmVal.tm_hour = hh;
    tmVal.tm_min = mm;
    tmVal.tm_sec = ss;
    tmVal.tm_isdst = -1;
    std::time_t expTime = std::mktime(&tmVal);
    if (expTime == static_cast<std::time_t>(-1)) {
        return true;
    }
    return expTime <= std::time(nullptr);
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
