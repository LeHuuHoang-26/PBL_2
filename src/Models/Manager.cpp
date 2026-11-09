#include "Manager.h"
#include "Human.h"
#include "libPBL2.h"
#include "../Utils/AppExceptions.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

using namespace std;

Manager::Manager()
    : Human(), username(""), password(""), role(ManagerRole::Empty), state(0) {}

Manager::Manager(ifstream &f) {
    string tmp;
    if (!getline(f, id, '|')) {
        this->setDefault();
        return;
    }
    libPBL2::trim(id);

    if (!getline(f, fullName, '|')) {
        this->setDefault();
        return;
    }
    libPBL2::trim(fullName);

    if (!getline(f, phone, '|')) {
        this->setDefault();
        return;
    }
    libPBL2::trim(phone);

    if (!getline(f, tmp, '|')) {
        this->setDefault();
        return;
    }
    try {
        gender = stoi(tmp);
    } catch (...) {
        gender = true;
    }

    if (!getline(f, tmp, '|')) {
        this->setDefault();
        return;
    }
    try {
        age = stoi(tmp);
    } catch (...) {
        age = 0;
    }

    if (!getline(f, username, '|')) {
        this->setDefault();
        return;
    }
    libPBL2::trim(username);

    if (!getline(f, password, '|')) {
        this->setDefault();
        return;
    }
    libPBL2::trim(password);

    if (!getline(f, tmp, '|')) {
        this->setDefault();
        return;
    }
    libPBL2::trim(tmp);
    role = roleFromString(tmp);

    if (!getline(f, tmp, '\n')) {
        this->setDefault();
        return;
    }
    libPBL2::trim(tmp);
    try {
        state = stoi(tmp);
    } catch (...) {
        state = 1;
    }
}

Manager::Manager(string id, string fullName, string phone, bool gender, int age, string username, string password, ManagerRole role, int state)
    : Human(id, fullName, phone, gender, age), username(username), password(password), role(role), state(state) {}

Manager::Manager(const Manager &M)
    : Human(M.id, M.fullName, M.phone, M.gender, M.age), username(M.username), password(M.password), role(M.role), state(M.state) {}

ManagerRole Manager::getRole() const { return role; }
string Manager::getUsername() const { return username; }
string Manager::getPassword() const { return password; }
int Manager::getState() const { return state; }

void Manager::setUsername(const string &un) { username = un; }
void Manager::setPassword(const string &pw) { password = pw; }
void Manager::setRole(ManagerRole r) { role = r; }
void Manager::setState(int st) { state = st; }

void Manager::setDefault() {
    id = "";
    fullName = "";
    phone = "";
    gender = 0;
    age = 0;
    username = "";
    password = "";
    role = ManagerRole::Empty;
    state = 0;
}

bool Manager::isDefault() const {
    return id.empty() && fullName.empty() && username.empty() && password.empty() && role == ManagerRole::Empty;
}

ostream &operator << (ostream &out, const Manager &M) {
    out << M.id << " " << M.fullName << " " << M.phone << " "
        << (M.gender ? "Nam" : "Nu") << " " << M.age << " "
        << M.username << " " << Manager::roleToString(M.role) << " " << M.state;
    return out;
}

Manager &Manager::operator = (const Manager &M) {
    if (this != &M) {
        this->id = M.id;
        this->fullName = M.fullName;
        this->phone = M.phone;
        this->gender = M.gender;
        this->age = M.age;
        this->username = M.username;
        this->password = M.password;
        this->role = M.role;
        this->state = M.state;
    }
    return *this;
}

bool Manager::isValid(const string &username, const string &password) const {
    return (this->username == username && this->password == password);
}

bool Manager::isActive() const {
    return (this->state == 1);
}

string Manager::roleToString(ManagerRole role) {
    switch (role) {
        case ManagerRole::Administrator: return "Administrator";
        case ManagerRole::Receptionist: return "Receptionist";
        case ManagerRole::SpecialReceptionist: return "SpecialReceptionist";
        default: return "Empty";
    }
}

ManagerRole Manager::roleFromString(const string &str) {
    if (str == "3" || str == "Administrator" || str == "Admin") return ManagerRole::Administrator;
    if (str == "2" || str == "SpecialReceptionist") return ManagerRole::SpecialReceptionist;
    if (str == "1" || str == "Receptionist") return ManagerRole::Receptionist;
    return ManagerRole::Empty;
}

string Manager::toString() const {
    string roleNum = "0";
    if (role == ManagerRole::Receptionist) roleNum = "1";
    else if (role == ManagerRole::SpecialReceptionist) roleNum = "2";
    else if (role == ManagerRole::Administrator) roleNum = "3";

    return libPBL2::padRightUtf8(id, 6) + " | "
         + libPBL2::padRightUtf8(fullName, 18) + " | "
         + libPBL2::padRightUtf8(phone, 10) + " | "
         + libPBL2::padRightUtf8(gender ? "1" : "0", 9) + " | "
         + libPBL2::padRightUtf8(to_string(age), 9) + " | "
         + libPBL2::padRightUtf8(username, 20) + " | "
         + libPBL2::padRightUtf8(password, 17) + " | "
         + libPBL2::padRightUtf8(roleNum, 7) + " | "
         + to_string(state);
}

Manager Manager::loadFromString(const string &str) {
    if (str.empty()) return Manager();
    stringstream ss(str);
    string id, fullName, phone, genderStr, ageStr, un, pw, roleStr, stateStr;

    if (!getline(ss, id, '|') ||
        !getline(ss, fullName, '|') ||
        !getline(ss, phone, '|') ||
        !getline(ss, genderStr, '|') ||
        !getline(ss, ageStr, '|') ||
        !getline(ss, un, '|') ||
        !getline(ss, pw, '|') ||
        !getline(ss, roleStr, '|') ||
        !getline(ss, stateStr)) {
        return Manager();
    }

    libPBL2::trim(id);
    libPBL2::trim(fullName);
    libPBL2::trim(phone);
    libPBL2::trim(genderStr);
    libPBL2::trim(ageStr);
    libPBL2::trim(un);
    libPBL2::trim(pw);
    libPBL2::trim(roleStr);
    libPBL2::trim(stateStr);

    if (id.empty()) return Manager();

    bool gender = (genderStr == "1");
    int age = 0;
    try {
        if (!ageStr.empty()) age = stoi(ageStr);
    } catch (...) {}
    ManagerRole r = roleFromString(roleStr);
    int st = 1;
    try {
        if (!stateStr.empty()) st = stoi(stateStr);
    } catch (...) {}

    return Manager(id, fullName, phone, gender, age, un, pw, r, st);
}

LinkList<Manager>* Manager::loadFromFile(const string &path) {
    ifstream f(path);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để đọc dữ liệu.");
    }

    LinkList<Manager> *ManagerList = nullptr;
    try {
        ManagerList = new LinkList<Manager>;
        string tmp;
        getline(f, tmp); // Bỏ qua tiêu đề
        getline(f, tmp); // Bỏ qua dòng phân cách

        while (f.peek() != EOF) {
            Manager M(f);
            if (M.isDefault())
                break;
            ManagerList->insert(M);
        }
        f.close();
        return ManagerList;
    } catch (...) {
        delete ManagerList;
        f.close();
        throw;
    }
}

bool Manager::saveToFile(const string &path, const LinkList<Manager> *list) {
    if (!list || list->getHeader() == nullptr)
        return false;

    ofstream f(path);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để ghi dữ liệu.");
    }

    f << "MA_ID  | MA_FullName        | MA_Phone   | MA_Gender | MA_Age    | MA_UserName          | MA_Password       | MA_Role | MA_State\n";
    f << "-------+--------------------+------------+-----------+-----------+----------------------+-------------------+---------+---------\n";

    LinkList<Manager>::Node *tmp = list->getHeader()->next;
    while (tmp != nullptr && tmp->next != nullptr) {
        if (!tmp->info.isDefault()) {
            f << tmp->info.toString() << "\n";
        }
        tmp = tmp->next;
    }

    f.close();
    return true;
}
