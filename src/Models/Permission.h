#ifndef PERMISSION_H
#define PERMISSION_H

#include <string>
#include "LinkList.h"

enum class PermissionType {
    Empty = 0,
    Per_1,
    Per_2,
    Per_3,
    Per_4,
    Per_5
};

class Permission {
private:
    std::string MA_id;
    PermissionType Per_id;

public:
    // Constructors & Destructor
    Permission(std::string maId = "", PermissionType perId = PermissionType::Empty);
    Permission(const Permission &other);
    Permission &operator=(const Permission &other) = default;
    ~Permission() = default;

    // Getters & Setters
    std::string getMA_id() const;
    PermissionType getPer_id() const;
    void setMA_id(const std::string &maId);
    void setPer_id(PermissionType perId);

    // State Helpers
    bool isDefault() const;
    void setDefault();

    // Type Conversions & Serialization
    static std::string permissionTypeToString(PermissionType per);
    static PermissionType permissionTypeFromString(const std::string &str);
    std::string toString() const;
    static Permission loadFromString(const std::string &str);

    // File I/O
    static LinkList<Permission>* loadFromFile(const std::string &path);
    static bool saveToFile(const std::string &path, const LinkList<Permission> *list);
};

#endif
