#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <string>

class LoginDialog : public QDialog {
    Q_OBJECT
private:
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QLabel *errorLabel;
    std::string loggedInFullName;
    int loggedInRole;

public:
    explicit LoginDialog(QWidget *parent = nullptr);

    std::string getLoggedInFullName() const { return loggedInFullName; }
    int getLoggedInRole() const { return loggedInRole; }

private slots:
    void onLoginClicked();
};

#endif // LOGINDIALOG_H
