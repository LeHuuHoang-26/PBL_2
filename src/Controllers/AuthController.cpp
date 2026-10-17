#include "AuthController.h"
#include "libPBL2.h"

AuthController::AuthController() {}

bool AuthController::login(const std::string &username, const std::string &password, Manager &loggedInUser, std::string &errorMessage) {
    if (username.empty() || password.empty()) {
        errorMessage = "Vui lòng nhập đầy đủ tên đăng nhập và mật khẩu!";
        return false;
    }

    LinkList<Manager> *managers = getAllManagers();
    if (!managers) {
        errorMessage = "Không thể kết nối đến cơ sở dữ liệu Manager.txt!";
        return false;
    }

    bool matched = false;
    auto *cur = managers->getHeader()->next;
    while (cur != managers->getFooter()) {
        if (cur->info.isValid(username, password)) {
            if (!cur->info.isActive()) {
                errorMessage = "Tài khoản của bạn hiện đang bị khóa!";
                delete managers;
                return false;
            }
            loggedInUser = cur->info;
            matched = true;
            break;
        }
        cur = cur->next;
    }

    delete managers;
    if (!matched) {
        errorMessage = "Sai tên đăng nhập hoặc mật khẩu!";
        return false;
    }

    return true;
}

LinkList<Manager>* AuthController::getAllManagers() {
    std::string path = libPBL2::getDataPath("Manager.txt");
    return Manager::loadFromFile(path);
}

std::vector<Manager> AuthController::searchManagers(const std::string &keyword) {
    std::vector<Manager> result;
    LinkList<Manager> *list = getAllManagers();
    if (!list) return result;

    auto *cur = list->getHeader()->next;
    while (cur != list->getFooter()) {
        const Manager &m = cur->info;
        if (keyword.empty() ||
            m.getfullName().find(keyword) != std::string::npos ||
            m.getUsername().find(keyword) != std::string::npos ||
            m.getID().find(keyword) != std::string::npos) {
            result.push_back(m);
        }
        cur = cur->next;
    }

    delete list;
    return result;
}
