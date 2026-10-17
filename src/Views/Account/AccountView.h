#ifndef ACCOUNTVIEW_H
#define ACCOUNTVIEW_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <string>
#include "AuthController.h"

class AccountView : public QWidget {
    Q_OBJECT
private:
    AuthController controller; // Tầng Controller quản lý tài khoản & phân quyền
    QLineEdit *searchEdit;
    QPushButton *btnSearch;
    QPushButton *btnRefresh;
    QTableWidget *tableAccounts;
    QLabel *statusLabel;

public:
    explicit AccountView(QWidget *parent = nullptr);
    void loadData(const std::string &keyword = "");

private slots:
    void onSearchClicked();
    void onRefreshClicked();
};

#endif // ACCOUNTVIEW_H
