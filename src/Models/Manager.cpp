#include "Manager.h"
#include "libPBL2.h"
#include "AppExceptions.h"
#include <fstream>
#include <sstream>

Manager::Manager()
    : Human(), username(""), password(""), role(ManagerRole::Empty), state(0) {}

Manager::Manager(std::string id, std::string fullName, std::string phone, bool gender, int age, std::string username, std::string password, ManagerRole role, int state)
    : Human(id, fullName, phone, gender, age), username(username), password(password), role(role), state(state) {}

Manager::Manager(const Manager &M)
    : Human(M.id, M.fullName, M.phone, M.gender, M.age), username(M.username), password(M.password), role(M.role), state(M.state) {}

Manager &Manager::operator=(const Manager &M) {
    if (this != &M) {
        id = M.id;
        fullName = M.fullName;
        phone = M.phone;
        gender = M.gender;
        age = M.age;
        username = M.username;
        password = M.password;
        role = M.role;
        state = M.state;
    }
    return *this;
}

ManagerRole Manager::getRole() const { return role; }
std::string Manager::getUsername() const { return username; }
std::string Manager::getPassword() const { return password; }
int Manager::getState() const { return state; }

void Manager::setUsername(const std::string &un) { username = un; }
void Manager::setPassword(const std::string &pw) { password = pw; }
void Manager::setRole(ManagerRole r) { role = r; }
void Manager::setState(int st) { state = st; }

void Manager::setDefault() {
    id.clear();
    fullName.clear();
    phone.clear();
    gender = false;
    age = 0;
    username.clear();
    password.clear();
    role = ManagerRole::Empty;
    state = 0;
}

bool Manager::isDefault() const {
    return id.empty() && fullName.empty() && username.empty() && password.empty() && (role == ManagerRole::Empty);
}

bool Manager::isValid(const std::string &username, const std::string &password) const {
    return (this->username == username && this->password == password);
}

bool Manager::isActive() const {
    return state == 1;
}

std::string Manager::toString() const {
    std::string roleNum = "0";
    if (role == ManagerRole::Receptionist) roleNum = "1";
    else if (role == ManagerRole::SpecialReceptionist) roleNum = "2";
    else if (role == ManagerRole::Administrator) roleNum = "3";

    return libPBL2::padRightUtf8(id, 6) + " | "
         + libPBL2::padRightUtf8(fullName, 18) + " | "
         + libPBL2::padRightUtf8(phone, 10) + " | "
         + libPBL2::padRightUtf8(gender ? "1" : "0", 9) + " | "
         + libPBL2::padRightUtf8(std::to_string(age), 9) + " | "
         + libPBL2::padRightUtf8(username, 20) + " | "
         + libPBL2::padRightUtf8(password, 17) + " | "
         + libPBL2::padRightUtf8(roleNum, 7) + " | "
         + std::to_string(state);
}

Manager Manager::loadFromString(const std::string &str) {
    if (str.empty()) return Manager();
    std::stringstream ss(str);
    std::string id, fullName, phone, genderStr, ageStr, un, pw, roleStr, stateStr;

    if (!std::getline(ss, id, '|') ||
        !std::getline(ss, fullName, '|') ||
        !std::getline(ss, phone, '|') ||
        !std::getline(ss, genderStr, '|') ||
        !std::getline(ss, ageStr, '|') ||
        !std::getline(ss, un, '|') ||
        !std::getline(ss, pw, '|') ||
        !std::getline(ss, roleStr, '|') ||
        !std::getline(ss, stateStr)) {
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
    int age = libPBL2::safeParseInt(ageStr, "Tuổi quản trị viên", 0);
    ManagerRole r = roleFromString(roleStr);
    int st = libPBL2::safeParseInt(stateStr, "Trạng thái tài khoản", 1);

    return Manager(id, fullName, phone, gender, age, un, pw, r, st);
}

LinkList<Manager>* Manager::loadFromFile(const std::string &path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để đọc dữ liệu.");
    }

    LinkList<Manager> *ManagerList = nullptr;
    try {
        ManagerList = new LinkList<Manager>;
        std::string tmp;
        std::getline(f, tmp);
        std::getline(f, tmp);

        std::string line;
        int lineNo = 2;
        while (std::getline(f, line)) {
            ++lineNo;
            libPBL2::trim(line);
            if (line.empty()) continue;
            try {
                Manager M = Manager::loadFromString(line);
                if (!M.isDefault()) {
                    ManagerList->insert(M);
                }
            } catch (const MemoryAllocationException &) {
                throw;
            } catch (const DataFormatException &e) {
                throw DataFormatException(path, lineNo, e.what());
            } catch (const AppException &) {
                throw;
            } catch (const std::exception &e) {
                throw DataFormatException(path, lineNo, e.what());
            }
        }
        f.close();
        return ManagerList;
    } catch (...) {
        delete ManagerList;
        f.close();
        throw;
    }
}

bool Manager::saveToFile(const std::string &path, const LinkList<Manager> *list) {
    if (!list || !list->getHeader()) return false;

    const std::string tempPath = path + ".tmp";
    std::ofstream f(tempPath);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp tạm để ghi dữ liệu.");
    }

    try {
        f << "M_ID  | FullName           | Phone      | Gender    | Age       | Username             | Password          | Role    | State\n";
        f << "------+--------------------+------------+-----------+-----------+----------------------+-------------------+---------+------\n";

        for (auto *tmp = list->getHeader()->next; tmp && tmp->next; tmp = tmp->next) {
            if (!tmp->info.isDefault()) {
                f << tmp->info.toString() << '\n';
            }
        }

        f.flush();
        const bool writeOk = f.good();
        f.close();
        if (!writeOk || f.fail()) {
            std::remove(tempPath.c_str());
            throw FileIOException(path, "Lỗi ghi dữ liệu quản lý viên xuống tệp tạm.");
        }
        libPBL2::safeAtomicCommit(tempPath, path);
        return true;
    } catch (...) {
        f.close();
        std::remove(tempPath.c_str());
        throw;
    }
}

ManagerRole Manager::roleFromString(const std::string &str) {
    if (str == "3" || str == "Administrator" || str == "Admin") return ManagerRole::Administrator;
    if (str == "2" || str == "SpecialReceptionist") return ManagerRole::SpecialReceptionist;
    if (str == "1" || str == "Receptionist") return ManagerRole::Receptionist;
    return ManagerRole::Empty;
}
