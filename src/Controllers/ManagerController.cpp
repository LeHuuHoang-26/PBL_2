#include "ManagerController.h"
#include "libPBL2.h"
#include <fstream>
#include <sstream>
#include <cstdio>

Manager *ManagerController::findById(LinkList<Manager> *managers, const std::string &id) {
    if (!managers || !managers->getHeader()) return nullptr;
    auto *node = managers->getHeader()->next;
    while (node && node->next) {
        if (node->info.getID() == id) return &node->info;
        node = node->next;
    }
    return nullptr;
}

int ManagerController::count(const LinkList<Manager> *managers) {
    return managers ? managers->count : 0;
}

QString ManagerController::roleName(ManagerRole role) {
    switch (role) {
        case ManagerRole::Administrator: return QStringLiteral("Quản trị viên");
        case ManagerRole::Receptionist: return QStringLiteral("Lễ tân");
        case ManagerRole::SpecialReceptionist: return QStringLiteral("Lễ tân chuyên trách");
        default: return QStringLiteral("Chưa phân vai trò");
    }
}

bool ManagerController::isRootAdmin(const Manager *m) {
    if (!m) return false;
    std::string u = m->getUsername();
    libPBL2::trim(u);
    std::string id = m->getID();
    libPBL2::trim(id);
    return u == "HoangMA001" || id == "MA_001" || m->getRole() == ManagerRole::Administrator;
}

// ==========================================
// In-Memory Permission Logic
// ==========================================
QDateTime ManagerController::permissionExpiry(const std::string &managerId, const std::string &permissionId, const LinkList<PermissionManager> *permissionRecords) {
    if (!permissionRecords || !permissionRecords->getHeader()) return QDateTime();
    auto *curr = permissionRecords->getHeader()->next;
    while (curr && curr->next) {
        if (curr->info.getMA_id() == managerId && curr->info.getPer_id() == permissionId && curr->info.isTemporary()) {
            QDateTime parsed = QDateTime::fromString(QString::fromUtf8(curr->info.getExpiresAt().c_str()), Qt::ISODate);
            if (!parsed.isValid()) parsed = QDateTime::fromString(QString::fromUtf8(curr->info.getExpiresAt().c_str()), "yyyy-MM-dd HH:mm:ss");
            return parsed;
        }
        curr = curr->next;
    }
    return QDateTime();
}

bool ManagerController::isTemporaryAdmin(const Manager *m, const LinkList<PermissionManager> *permissionRecords) {
    if (!m || isRootAdmin(m)) return false;
    QDateTime expiry = permissionExpiry(m->getID(), "Per_1", permissionRecords);
    return expiry.isValid() && expiry > QDateTime::currentDateTime();
}

bool ManagerController::isAdministrator(const Manager *m, const LinkList<PermissionManager> *permissionRecords) {
    return isRootAdmin(m) || isTemporaryAdmin(m, permissionRecords);
}

bool ManagerController::hasPermission(const Manager *m, const std::string &permissionId, const LinkList<PermissionManager> *permissionRecords) {
    if (!m) return false;
    if (isRootAdmin(m) || isTemporaryAdmin(m, permissionRecords)) return true;
    if (permissionId == "Per_1") return false;

    if (!permissionRecords || !permissionRecords->getHeader()) return false;
    auto *curr = permissionRecords->getHeader()->next;
    while (curr && curr->next) {
        if (curr->info.getMA_id() == m->getID() && curr->info.getPer_id() == permissionId) {
            if (curr->info.isTemporary() && curr->info.isExpired()) {
                return false;
            }
            return true;
        }
        curr = curr->next;
    }
    return false;
}

QStringList ManagerController::getGrantedPermissions(const std::string &managerId, const LinkList<PermissionManager> *permissionRecords) {
    QStringList granted;
    if (!permissionRecords || !permissionRecords->getHeader()) return granted;
    auto *curr = permissionRecords->getHeader()->next;
    while (curr && curr->next) {
        if (curr->info.getMA_id() == managerId) {
            if (curr->info.isTemporary() && curr->info.isExpired()) {
                curr = curr->next;
                continue;
            }
            granted.append(QString::fromUtf8(curr->info.getPer_id().c_str()));
        }
        curr = curr->next;
    }
    return granted;
}

bool ManagerController::cleanExpiredPermissions(LinkList<PermissionManager> *permissionRecords) {
    if (!permissionRecords || !permissionRecords->getHeader()) return false;
    bool hasExpired = false;
    auto *curr = permissionRecords->getHeader()->next;
    while (curr && curr->next) {
        auto *next = curr->next;
        if (curr->info.isTemporary() && curr->info.isExpired()) {
            permissionRecords->remove(curr);
            hasExpired = true;
        }
        curr = next;
    }
    return hasExpired;
}

bool ManagerController::savePermissions(const std::string &managerId, const LinkList<PermissionManager> &grantsForManager, LinkList<PermissionManager> *permissionRecords) {
    if (!permissionRecords || !permissionRecords->getHeader()) return false;

    // Xóa toàn bộ các quyền cũ của managerId trong danh sách
    auto *curr = permissionRecords->getHeader()->next;
    while (curr && curr->next) {
        auto *next = curr->next;
        if (curr->info.getMA_id() == managerId) {
            permissionRecords->remove(curr);
        }
        curr = next;
    }

    // Nạp các quyền mới được cấp
    if (grantsForManager.getHeader()) {
        auto *grantNode = grantsForManager.getHeader()->next;
        while (grantNode && grantNode->next) {
            permissionRecords->insert(grantNode->info);
            grantNode = grantNode->next;
        }
    }
    return true;
}

std::string ManagerController::generateNextId(const LinkList<Manager> *managers) {
    int maximum = 0;
    if (managers && managers->getHeader()) {
        auto *n = managers->getHeader()->next;
        while (n && n->next) {
            std::string id = n->info.getID();
            if (id.rfind("MA_", 0) == 0) {
                try {
                    int val = std::stoi(id.substr(3));
                    if (val > maximum) maximum = val;
                } catch (const std::invalid_argument &) {
                    // Ký tự ID không hợp lệ, bỏ qua
                } catch (const std::out_of_range &) {
                    // ID vượt quá giá trị số nguyên, bỏ qua
                }
            }
            n = n->next;
        }
    }
    char buf[32];
    std::snprintf(buf, sizeof(buf), "MA_%03d", maximum + 1);
    return std::string(buf);
}

// ==========================================
// File-based Overloads (Backward Compatibility)
// ==========================================
QDateTime ManagerController::permissionExpiry(const std::string &managerId, const std::string &permissionId, const std::string &filePath) {
    LinkList<PermissionManager> *list = nullptr;
    try {
        list = PermissionManager::loadFromFile(filePath);
    } catch (const std::exception &) {
        return QDateTime();
    }
    QDateTime res = permissionExpiry(managerId, permissionId, list);
    delete list;
    return res;
}

bool ManagerController::isTemporaryAdmin(const Manager *m, const std::string &filePath) {
    if (!m || isRootAdmin(m)) return false;
    QDateTime expiry = permissionExpiry(m->getID(), "Per_1", filePath);
    return expiry.isValid() && expiry > QDateTime::currentDateTime();
}

bool ManagerController::isAdministrator(const Manager *m, const std::string &filePath) {
    return isRootAdmin(m) || isTemporaryAdmin(m, filePath);
}

bool ManagerController::hasPermission(const Manager *m, const std::string &permissionId, const std::string &filePath) {
    if (!m) return false;
    if (isRootAdmin(m) || isTemporaryAdmin(m, filePath)) return true;
    if (permissionId == "Per_1") return false;

    LinkList<PermissionManager> *list = nullptr;
    try {
        list = PermissionManager::loadFromFile(filePath);
    } catch (const std::exception &) {
        return false;
    }
    bool res = hasPermission(m, permissionId, list);
    delete list;
    return res;
}

QStringList ManagerController::getGrantedPermissions(const std::string &managerId, const std::string &filePath) {
    LinkList<PermissionManager> *list = nullptr;
    try {
        list = PermissionManager::loadFromFile(filePath);
    } catch (const std::exception &) {
        return QStringList();
    }
    QStringList res = getGrantedPermissions(managerId, list);
    delete list;
    return res;
}

bool ManagerController::cleanExpiredPermissions(const std::string &filePath) {
    LinkList<PermissionManager> *list = nullptr;
    try {
        list = PermissionManager::loadFromFile(filePath);
    } catch (const std::exception &) {
        return false;
    }
    bool cleaned = cleanExpiredPermissions(list);
    if (cleaned) {
        try {
            PermissionManager::saveToFile(filePath, list);
        } catch (const std::exception &) {
            delete list;
            return false;
        }
    }
    delete list;
    return true;
}

bool ManagerController::savePermissions(const std::string &managerId, const LinkList<PermissionManager> &grantsForManager, const std::string &filePath) {
    LinkList<PermissionManager> *list = nullptr;
    try {
        list = PermissionManager::loadFromFile(filePath);
    } catch (const std::exception &) {
        list = new LinkList<PermissionManager>;
    }
    bool ok = savePermissions(managerId, grantsForManager, list);
    if (ok) {
        try {
            PermissionManager::saveToFile(filePath, list);
        } catch (const std::exception &) {
            ok = false;
        }
    }
    delete list;
    return ok;
}
