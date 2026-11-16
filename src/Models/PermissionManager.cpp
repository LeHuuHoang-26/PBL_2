#include "PermissionManager.h"
#include "libPBL2.h"
#include "../Utils/AppExceptions.h"
#include <fstream>
#include <sstream>
#include <QDateTime>
#include <QString>

using namespace std;

// ==========================================
// Constructors & Destructor
// ==========================================
PermissionManager::PermissionManager()
    : MA_id(""), Per_id(""), Expires_At("") {}

PermissionManager::PermissionManager(string maId, string perId, string expiresAt)
    : MA_id(maId), Per_id(perId), Expires_At(expiresAt) {}

PermissionManager::PermissionManager(const PermissionManager &other)
    : MA_id(other.MA_id), Per_id(other.Per_id), Expires_At(other.Expires_At) {}

// ==========================================
// Getters & Setters
// ==========================================
string PermissionManager::getMA_id() const { return MA_id; }
string PermissionManager::getPer_id() const { return Per_id; }
string PermissionManager::getExpiresAt() const { return Expires_At; }

void PermissionManager::setMA_id(const string &maId) { MA_id = maId; }
void PermissionManager::setPer_id(const string &perId) { Per_id = perId; }
void PermissionManager::setExpiresAt(const string &expiresAt) { Expires_At = expiresAt; }

// ==========================================
// Helpers
// ==========================================
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
    QDateTime parsed = QDateTime::fromString(QString::fromUtf8(Expires_At.c_str()), Qt::ISODate);
    if (!parsed.isValid()) {
        parsed = QDateTime::fromString(QString::fromUtf8(Expires_At.c_str()), "yyyy-MM-dd HH:mm:ss");
    }
    return parsed.isValid() && parsed <= QDateTime::currentDateTime();
}

// ==========================================
// Serialization & File I/O
// ==========================================
string PermissionManager::toString() const {
    return MA_id + " | " + Per_id + " | " + Expires_At;
}

PermissionManager PermissionManager::loadFromString(const string &str) {
    if (str.empty()) return PermissionManager();
    stringstream ss(str);
    string maId, perId, expiry;
    if (!getline(ss, maId, '|') || !getline(ss, perId, '|')) {
        return PermissionManager();
    }
    getline(ss, expiry);
    libPBL2::trim(maId);
    libPBL2::trim(perId);
    libPBL2::trim(expiry);

    if (maId.empty() || maId == "MA_ID" || maId.find('-') != string::npos) {
        return PermissionManager();
    }

    return PermissionManager(maId, perId, expiry);
}

LinkList<PermissionManager>* PermissionManager::loadFromFile(const string &path) {
    ifstream in(path);
    if (!in.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để đọc dữ liệu phân quyền.");
    }

    LinkList<PermissionManager> *list = new LinkList<PermissionManager>;
    string line;
    // Bỏ qua dòng tiêu đề và phân cách nếu có
    if (getline(in, line)) {
        if (line.find("MA_ID") != string::npos) {
            getline(in, line); // Bỏ qua dòng -------+--------+...
        } else {
            // Nếu không phải header thì xử lý như dữ liệu
            libPBL2::trim(line);
            if (!line.empty()) {
                PermissionManager pm = loadFromString(line);
                if (!pm.isDefault()) list->insert(pm);
            }
        }
    }

    while (getline(in, line)) {
        libPBL2::trim(line);
        if (line.empty()) continue;
        PermissionManager pm = loadFromString(line);
        if (!pm.isDefault()) {
            list->insert(pm);
        }
    }

    return list;
}

bool PermissionManager::saveToFile(const string &path, const LinkList<PermissionManager> *list) {
    ofstream out(path);
    if (!out.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để ghi dữ liệu phân quyền.");
    }

    out << "MA_ID | Per_ID | Expires_At\n";
    out << "-------+--------+--------------------\n";
    if (list && list->getHeader()) {
        auto *curr = list->getHeader()->next;
        while (curr && curr->next) {
            out << curr->info.toString() << "\n";
            curr = curr->next;
        }
    }
    return true;
}
