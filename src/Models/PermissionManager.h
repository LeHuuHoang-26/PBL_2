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
    // Constructors & Destructor
    PermissionManager();
    PermissionManager(std::string maId, std::string perId, std::string expiresAt = "");
    PermissionManager(const PermissionManager &other);
    PermissionManager &operator=(const PermissionManager &other) = default;
    ~PermissionManager() = default;

    // Getters & Setters
    std::string getMA_id() const;
    std::string getPer_id() const;
    std::string getExpiresAt() const;

    void setMA_id(const std::string &maId);
    void setPer_id(const std::string &perId);
    void setExpiresAt(const std::string &expiresAt);

    // Helpers
    bool isDefault() const;
    void setDefault();
    bool isTemporary() const;
    bool isExpired() const;

    // Serialization & File I/O
    std::string toString() const;
    static PermissionManager loadFromString(const std::string &str);

    static LinkList<PermissionManager>* loadFromFile(const std::string &path);
    static bool saveToFile(const std::string &path, const LinkList<PermissionManager> *list);
};

#endif // PERMISSIONMANAGER_H
