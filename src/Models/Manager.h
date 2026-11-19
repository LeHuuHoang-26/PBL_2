#ifndef MANAGER_H
#define MANAGER_H

#include <string>
#include "Human.h"
#include "LinkList.h"

enum class ManagerRole {
    Empty,
    Receptionist,
    SpecialReceptionist,
    Administrator
};

class Manager : public Human {
private:
    std::string username;
    std::string password;
    ManagerRole role;
    int state;

public:
    Manager();
    Manager(std::string id, std::string fullName, std::string phone, bool gender, int age, std::string username, std::string password, ManagerRole role, int state);
    Manager(const Manager &M);
    Manager(Manager &&M) noexcept = default;
    Manager &operator=(const Manager &M);
    Manager &operator=(Manager &&M) noexcept = default;
    ~Manager() = default;

    ManagerRole getRole() const;
    std::string getUsername() const;
    std::string getPassword() const;
    int getState() const;

    void setUsername(const std::string &un);
    void setPassword(const std::string &pw);
    void setRole(ManagerRole r);
    void setState(int st);

    void setDefault();
    bool isDefault() const;
    bool isValid(const std::string &username, const std::string &password) const;
    bool isActive() const;

    std::string toString() const;
    static Manager loadFromString(const std::string &str);
    static LinkList<Manager>* loadFromFile(const std::string &path);
    static bool saveToFile(const std::string &path, const LinkList<Manager> *list);
    static ManagerRole roleFromString(const std::string &str);
};

#endif
