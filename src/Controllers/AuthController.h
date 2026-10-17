#ifndef AUTHCONTROLLER_H
#define AUTHCONTROLLER_H

#include "Manager.h"
#include "LinkList.h"
#include <string>
#include <vector>

class AuthController {
public:
    AuthController();

    // Xác thực đăng nhập: kiểm tra username, password, trạng thái tài khoản
    bool login(const std::string &username, const std::string &password, Manager &loggedInUser, std::string &errorMessage);

    // Lấy toàn bộ danh sách tài khoản
    LinkList<Manager>* getAllManagers();

    // Tìm kiếm tài khoản theo từ khóa (tên, username, mã)
    std::vector<Manager> searchManagers(const std::string &keyword);
};

#endif // AUTHCONTROLLER_H
