#include <QApplication>
#include "MainWindow.h"
#include "LoginDialog.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // 1. Mở hộp thoại Đăng nhập đầu tiên
    LoginDialog login;
    if (login.exec() == QDialog::Accepted) {
        // 2. Đăng nhập thành công -> Khởi tạo và hiển thị Cửa sổ chính với thông tin tài khoản
        MainWindow *window = new MainWindow(login.getLoggedInFullName(), login.getLoggedInRole());
        window->show();

        // 3. Chạy vòng lặp sự kiện chính của ứng dụng Qt
        return app.exec();
    }

    // Nếu người dùng bấm [Thoát] ở màn hình đăng nhập thì tắt app
    return 0;
}
