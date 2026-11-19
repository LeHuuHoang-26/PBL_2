#ifndef PERMISSION_H
#define PERMISSION_H

#include <string>
#include "LinkList.h"

class Permission {
private:
    std::string Per_id;
    std::string Per_Name;
    std::string description;

public:
    Permission(std::string perId = "", std::string perName = "", std::string desc = "");
    Permission(const Permission &other);
    Permission &operator=(const Permission &other) = default;
    Permission(Permission &&other) noexcept = default;
    Permission &operator=(Permission &&other) noexcept = default;
    ~Permission() = default;

    std::string getPer_id() const;
    std::string getPer_Name() const;
    std::string getDescription() const;

    void setPer_id(const std::string &id);
    void setPer_Name(const std::string &name);
    void setDescription(const std::string &desc);

    bool isDefault() const;
    void setDefault();

    static LinkList<Permission>* getSystemCatalog();
};

#endif
