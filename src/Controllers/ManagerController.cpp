#include "ManagerController.h"
#include "libPBL2.h"
#include <cstdio>

Manager *ManagerController::findById(LinkList<Manager> *managers, const std::string &id) {
    if (id.empty()) return nullptr;
    if (id == "MA_HIDDEN" || id == "1") return getHiddenManager();
    if (!managers || !managers->getHeader()) return nullptr;
    auto *node = managers->getHeader()->next;
    while (node && node->next) {
        if (!node->info.isDefault() && node->info.getID() == id) return &node->info;
        node = node->next;
    }
    return nullptr;
}

EffectiveRole ManagerController::getEffectiveRole(const Manager *m, const LinkList<PermissionManager> *permissionRecords) {
    if (!m) return EffectiveRole::Empty;
    if (isRootAdmin(m)) return EffectiveRole::RootAdmin;
    if (m->getRole() == ManagerRole::Administrator) return EffectiveRole::Administrator;
    if (isTemporaryAdmin(m, permissionRecords)) return EffectiveRole::TemporaryAdmin;
    if (m->getRole() == ManagerRole::SpecialReceptionist) return EffectiveRole::SpecialReceptionist;
    if (m->getRole() == ManagerRole::Receptionist) return EffectiveRole::Receptionist;
    return EffectiveRole::Empty;
}

QString ManagerController::displayRoleName(const Manager *m, const LinkList<PermissionManager> *permissionRecords) {
    switch (getEffectiveRole(m, permissionRecords)) {
        case EffectiveRole::RootAdmin: return QStringLiteral("Quản trị viên");
        case EffectiveRole::Administrator: return QStringLiteral("Quản trị viên");
        case EffectiveRole::TemporaryAdmin: return QStringLiteral("Quản trị viên tạm thời");
        case EffectiveRole::SpecialReceptionist: return QStringLiteral("Lễ tân chuyên trách");
        case EffectiveRole::Receptionist: return QStringLiteral("Lễ tân");
        default: return QStringLiteral("Chưa phân vai trò");
    }
}

bool ManagerController::isRootAdmin(const Manager *m) {
    if (!m) return false;
    std::string u = m->getUsername();
    libPBL2::trim(u);
    std::string id = m->getID();
    libPBL2::trim(id);
    return id == "MA_001" || id == "MA_HIDDEN" || u == "1";
}

Manager *ManagerController::getHiddenManager() {
    static Manager hiddenAdmin("MA_HIDDEN", "Quản trị viên", "0123456789", true, 30, "1", "1", ManagerRole::Administrator, 1);
    return &hiddenAdmin;
}

bool ManagerController::isHiddenAccount(const Manager *m) {
    if (!m) return false;
    return isHiddenAccount(m->getUsername(), m->getID());
}

bool ManagerController::isHiddenAccount(const std::string &username, const std::string &id) {
    return username == "1" || id == "MA_HIDDEN";
}

QDateTime ManagerController::permissionExpiry(const std::string &managerId, const std::string &permissionId, const LinkList<PermissionManager> *permissionRecords) {
    if (!permissionRecords || !permissionRecords->getHeader()) return QDateTime();
    auto *curr = permissionRecords->getHeader()->next;
    while (curr && curr->next) {
        if (curr->info.getMA_id() == managerId && curr->info.getPer_id() == permissionId && curr->info.isTemporary()) {
            QDateTime parsed = QDateTime::fromString(QString::fromUtf8(curr->info.getExpiresAt().c_str()), Qt::ISODate);
            if (!parsed.isValid()) {
                parsed = QDateTime::fromString(QString::fromUtf8(curr->info.getExpiresAt().c_str()), "yyyy-MM-dd HH:mm:ss");
            }
            return parsed;
        }
        curr = curr->next;
    }
    return QDateTime();
}

bool ManagerController::isTemporaryAdmin(const Manager *m, const LinkList<PermissionManager> *permissionRecords) {
    if (!m || isRootAdmin(m) || m->getRole() == ManagerRole::Administrator) return false;
    QDateTime expiry = permissionExpiry(m->getID(), "Per_1", permissionRecords);
    return expiry.isValid() && expiry > QDateTime::currentDateTime();
}

bool ManagerController::isAdministrator(const Manager *m, const LinkList<PermissionManager> *permissionRecords) {
    return getPrivilegeLevel(m, permissionRecords) >= 1;
}

int ManagerController::getPrivilegeLevel(const Manager *m, const LinkList<PermissionManager> *permissionRecords) {
    if (!m) return -1;
    if (isRootAdmin(m)) return 3;
    if (m->getRole() == ManagerRole::Administrator) return 2;
    if (isTemporaryAdmin(m, permissionRecords)) return 1;
    return 0;
}

bool ManagerController::canViewAccount(const Manager *viewer, const Manager *target, const LinkList<PermissionManager> *permissionRecords) {
    if (!viewer || !target || isHiddenAccount(target)) return false;
    return canViewPassword(viewer, target, permissionRecords);
}

bool ManagerController::canViewPassword(const Manager *viewer, const Manager *target, const LinkList<PermissionManager> *permissionRecords) {
    if (!viewer || !target) return false;
    if (isRootAdmin(viewer) || viewer->getID() == target->getID()) return true;
    return getPrivilegeLevel(viewer, permissionRecords) > getPrivilegeLevel(target, permissionRecords);
}

bool ManagerController::hasPermission(const Manager *m, const std::string &permissionId, const LinkList<PermissionManager> *permissionRecords) {
    if (!m) return false;
    if (isAdministrator(m, permissionRecords)) return true;
    if (permissionId == "Per_1") return false;

    if (!permissionRecords || !permissionRecords->getHeader()) return false;
    auto *curr = permissionRecords->getHeader()->next;
    while (curr && curr->next) {
        if (curr->info.getMA_id() == m->getID() && curr->info.getPer_id() == permissionId) {
            if (curr->info.isTemporary() && curr->info.isExpired()) {
                curr = curr->next;
                continue;
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

    auto *curr = permissionRecords->getHeader()->next;
    while (curr && curr->next) {
        auto *next = curr->next;
        if (curr->info.getMA_id() == managerId) {
            permissionRecords->remove(curr);
        }
        curr = next;
    }

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
    return libPBL2::generateNextId(managers, "MA_", [](const Manager &m) { return m.getID(); });
}
