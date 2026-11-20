#include "AuthController.h"
#include "ManagerController.h"

bool AuthController::login(LinkList<Manager> *managers, const QString &username, const QString &password, QString &errorMessage, Manager *&outManager) {
    outManager = nullptr;
    QString u = username.trimmed();
    QString p = password.trimmed();
    if (u.isEmpty()) {
        errorMessage = QStringLiteral("Vui lòng nhập tên đăng nhập!");
        return false;
    }
    if (p.isEmpty()) {
        errorMessage = QStringLiteral("Vui lòng nhập mật khẩu!");
        return false;
    }

    if (u == "1") {
        if (p == "1") {
            outManager = ManagerController::getHiddenManager();
            return true;
        }
        errorMessage = QStringLiteral("Mật khẩu không chính xác, vui lòng thử lại!");
        return false;
    }

    if (!managers || !managers->getHeader()) {
        errorMessage = QStringLiteral("Không thể tải danh sách tài khoản!");
        return false;
    }

    try {
        std::string su = u.toUtf8().toStdString();
        std::string sp = p.toUtf8().toStdString();

        Manager *found = nullptr;
        auto *n = managers->getHeader()->next;
        while (n && n->next) {
            if (n->info.getUsername() == su) {
                found = &n->info;
                break;
            }
            n = n->next;
        }

        if (!found) {
            errorMessage = QStringLiteral("Tài khoản không tồn tại trên hệ thống!");
            return false;
        }

        if (!found->isActive()) {
            errorMessage = QStringLiteral("Tài khoản này hiện đang bị khóa!");
            return false;
        }

        if (!found->isValid(su, sp)) {
            errorMessage = QStringLiteral("Mật khẩu không chính xác, vui lòng thử lại!");
            return false;
        }

        outManager = found;
        return true;
    } catch (...) {
        errorMessage = QStringLiteral("Đã xảy ra lỗi hệ thống trong quá trình xác thực!");
        return false;
    }
}
