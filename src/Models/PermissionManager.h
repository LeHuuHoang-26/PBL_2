#ifndef PERMISSIONMANAGER_H
#define PERMISSIONMANAGER_H

#include <string>
#include "LinkList.h"

class PermissionManager {
private:
    std::string MA_id;
    std::string Per_id;
    std::string Expires_At;

public:
    PermissionManager();
    PermissionManager(std::string maId, std::string perId, std::string expiresAt = "");
    PermissionManager(const PermissionManager &other);
    PermissionManager &operator=(const PermissionManager &other) = default;
    PermissionManager(PermissionManager &&other) noexcept = default;
    PermissionManager &operator=(PermissionManager &&other) noexcept = default;
    ~PermissionManager() = default;

    std::string getMA_id() const;
    std::string getPer_id() const;
    std::string getExpiresAt() const;

    bool isDefault() const;
    void setDefault();
    bool isTemporary() const;
    bool isExpired() const;

    std::string toString() const;
    static PermissionManager loadFromString(const std::string &str);

    static LinkList<PermissionManager>* loadFromFile(const std::string &path);
    static bool saveToFile(const std::string &path, const LinkList<PermissionManager> *list);
};

#endif
