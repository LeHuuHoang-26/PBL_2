#ifndef MANAGERCONTROLLER_H
#define MANAGERCONTROLLER_H

#include "../Models/Manager.h"
#include "../Models/PermissionManager.h"
#include "../DataStructures/LinkList.h"
#include <string>
#include <QString>
#include <QStringList>
#include <QList>
#include <QDateTime>

class ManagerController {
public:
    static Manager *findById(LinkList<Manager> *managers, const std::string &id);
    static QString roleName(ManagerRole role);
    static bool isRootAdmin(const Manager *m);
    static Manager *getHiddenManager();
    static bool isHiddenAccount(const Manager *m);
    static bool isHiddenAccount(const std::string &username, const std::string &id = "");

    // --- Xử lý nghiệp vụ phân quyền trên bộ nhớ RAM (In-Memory Model) ---
    static QDateTime permissionExpiry(const std::string &managerId, const std::string &permissionId, const LinkList<PermissionManager> *permissionRecords);
    static bool isTemporaryAdmin(const Manager *m, const LinkList<PermissionManager> *permissionRecords);
    static bool isAdministrator(const Manager *m, const LinkList<PermissionManager> *permissionRecords);
    static bool hasPermission(const Manager *m, const std::string &permissionId, const LinkList<PermissionManager> *permissionRecords);
    static QStringList getGrantedPermissions(const std::string &managerId, const LinkList<PermissionManager> *permissionRecords);
    static bool cleanExpiredPermissions(LinkList<PermissionManager> *permissionRecords);
    static bool savePermissions(const std::string &managerId, const LinkList<PermissionManager> &grantsForManager, LinkList<PermissionManager> *permissionRecords);

    static std::string generateNextId(const LinkList<Manager> *managers);
};

#endif // MANAGERCONTROLLER_H
