#include "AccountView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

AccountView::AccountView(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Tiêu đề
    QLabel *titleLabel = new QLabel("<h2>🔐 Quản Lý Tài Khoản Quản Trị (Admin Only)</h2>", this);
    mainLayout->addWidget(titleLabel);

    // Thanh tìm kiếm
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Tìm kiếm theo tên hoặc tên đăng nhập...");
    btnSearch = new QPushButton("🔍 Tìm kiếm", this);
    btnRefresh = new QPushButton("🔄 Làm mới", this);

    searchLayout->addWidget(searchEdit, 1);
    searchLayout->addWidget(btnSearch);
    searchLayout->addWidget(btnRefresh);
    mainLayout->addLayout(searchLayout);

    // Bảng dữ liệu tài khoản
    tableAccounts = new QTableWidget(this);
    tableAccounts->setColumnCount(6);
    tableAccounts->setHorizontalHeaderLabels({
        "Mã QL", "Họ và Tên", "Số ĐT", "Tên Đăng Nhập", "Chức Vụ", "Trạng Thái"
    });
    tableAccounts->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableAccounts->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableAccounts->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(tableAccounts);

    // Dòng trạng thái
    statusLabel = new QLabel(this);
    mainLayout->addWidget(statusLabel);

    connect(btnSearch, &QPushButton::clicked, this, &AccountView::onSearchClicked);
    connect(btnRefresh, &QPushButton::clicked, this, &AccountView::onRefreshClicked);
    connect(searchEdit, &QLineEdit::returnPressed, this, &AccountView::onSearchClicked);

    loadData();
}

void AccountView::loadData(const std::string &keyword) {
    // Gọi Controller tìm kiếm và lấy danh sách tài khoản
    std::vector<Manager> managers = controller.searchManagers(keyword);

    tableAccounts->setRowCount(0);
    for (size_t i = 0; i < managers.size(); ++i) {
        const Manager &m = managers[i];
        int row = (int)i;

        QString roleStr = "Không xác định";
        if (m.getRole() == ManagerRole::Administrator) roleStr = "Quản trị viên";
        else if (m.getRole() == ManagerRole::Receptionist) roleStr = "Tiếp tân";
        else if (m.getRole() == ManagerRole::SpecialReceptionist) roleStr = "Tiếp tân nâng cao";

        tableAccounts->insertRow(row);
        tableAccounts->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(m.getID())));
        tableAccounts->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(m.getfullName())));
        tableAccounts->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(m.getPhone())));
        tableAccounts->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(m.getUsername())));
        tableAccounts->setItem(row, 4, new QTableWidgetItem(roleStr));
        tableAccounts->setItem(row, 5, new QTableWidgetItem(m.isActive() ? "Đang hoạt động" : "Bị khóa"));
    }

    statusLabel->setText(QString("Hiển thị %1 tài khoản.").arg(managers.size()));
}

void AccountView::onSearchClicked() {
    loadData(searchEdit->text().trimmed().toStdString());
}

void AccountView::onRefreshClicked() {
    searchEdit->clear();
    loadData();
}
