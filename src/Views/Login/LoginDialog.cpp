#include "LoginDialog.h"
#include "AuthController.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent), loggedInRole(0) {
    setWindowTitle("Đăng Nhập Hệ Thống - MngCareBook");
    setFixedSize(380, 260);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *title = new QLabel("<h2 style='text-align: center; color: #1E3A8A;'>🏥 MngCareBook</h2>"
                                "<p style='text-align: center; color: #64748B;'>Vui lòng đăng nhập để tiếp tục</p>", this);
    layout->addWidget(title);

    usernameEdit = new QLineEdit(this);
    usernameEdit->setPlaceholderText("Tên đăng nhập (VD: HoangMA001)");
    layout->addWidget(usernameEdit);

    passwordEdit = new QLineEdit(this);
    passwordEdit->setPlaceholderText("Mật khẩu");
    passwordEdit->setEchoMode(QLineEdit::Password);
    layout->addWidget(passwordEdit);

    errorLabel = new QLabel(this);
    errorLabel->setStyleSheet("color: red; font-size: 12px;");
    layout->addWidget(errorLabel);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnLogin = new QPushButton("Đăng Nhập", this);
    QPushButton *btnCancel = new QPushButton("Thoát", this);

    btnLogin->setStyleSheet("background-color: #2563EB; color: white; padding: 8px 16px; font-weight: bold; border-radius: 4px;");
    btnCancel->setStyleSheet("padding: 8px 16px; border-radius: 4px;");

    btnLayout->addWidget(btnLogin);
    btnLayout->addWidget(btnCancel);
    layout->addLayout(btnLayout);

    connect(btnLogin, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
}

void LoginDialog::onLoginClicked() {
    std::string user = usernameEdit->text().trimmed().toStdString();
    std::string pass = passwordEdit->text().toStdString();

    AuthController auth;
    Manager loggedInUser;
    std::string errorMsg;

    if (auth.login(user, pass, loggedInUser, errorMsg)) {
        loggedInFullName = loggedInUser.getfullName();
        loggedInRole = (int)loggedInUser.getRole();
        accept(); // Đăng nhập thành công -> chuyển sang MainWindow
    } else {
        errorLabel->setText(QString::fromStdString(errorMsg));
    }
}
