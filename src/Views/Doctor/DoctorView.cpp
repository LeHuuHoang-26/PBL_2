#include "DoctorView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

DoctorView::DoctorView(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Tiêu đề
    QLabel *titleLabel = new QLabel("<h2>👨‍⚕️ Quản Lý Bác Sĩ</h2>", this);
    mainLayout->addWidget(titleLabel);

    // Thanh tìm kiếm
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Tìm kiếm theo tên hoặc mã bác sĩ...");
    btnSearch = new QPushButton("🔍 Tìm kiếm", this);
    btnRefresh = new QPushButton("🔄 Làm mới", this);

    searchLayout->addWidget(searchEdit, 1);
    searchLayout->addWidget(btnSearch);
    searchLayout->addWidget(btnRefresh);
    mainLayout->addLayout(searchLayout);

    // Bảng dữ liệu bác sĩ
    tableDoctors = new QTableWidget(this);
    tableDoctors->setColumnCount(8);
    tableDoctors->setHorizontalHeaderLabels({
        "Mã BS", "Họ và Tên", "Số ĐT", "Giới tính", "Tuổi", "Phòng", "Giá khám", "Đánh giá"
    });
    tableDoctors->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableDoctors->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableDoctors->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(tableDoctors);

    // Dòng trạng thái
    statusLabel = new QLabel(this);
    mainLayout->addWidget(statusLabel);

    connect(btnSearch, &QPushButton::clicked, this, &DoctorView::onSearchClicked);
    connect(btnRefresh, &QPushButton::clicked, this, &DoctorView::onRefreshClicked);
    connect(searchEdit, &QLineEdit::returnPressed, this, &DoctorView::onSearchClicked);

    loadData();
}

void DoctorView::loadData(const std::string &keyword) {
    // Gọi Controller xử lý tìm kiếm và lấy dữ liệu
    std::vector<Doctor> doctors = controller.searchDoctors(keyword);
    int total = controller.getTotalDoctorCount();

    tableDoctors->setRowCount(0);
    for (size_t i = 0; i < doctors.size(); ++i) {
        const Doctor &d = doctors[i];
        int row = (int)i;

        tableDoctors->insertRow(row);
        tableDoctors->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(d.getID())));
        tableDoctors->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(d.getfullName())));
        tableDoctors->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(d.getPhone())));
        tableDoctors->setItem(row, 3, new QTableWidgetItem(d.getGender() ? "Nam" : "Nữ"));
        tableDoctors->setItem(row, 4, new QTableWidgetItem(QString::number(d.getAge())));
        tableDoctors->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(d.getRoomId())));
        tableDoctors->setItem(row, 6, new QTableWidgetItem(QString::number((long long)d.getFee()) + " VNĐ"));
        tableDoctors->setItem(row, 7, new QTableWidgetItem(QString::number(d.getRating(), 'f', 1) + " ⭐"));
    }

    statusLabel->setText(QString("Hiển thị %1 / %2 bác sĩ.").arg(doctors.size()).arg(total));
}

void DoctorView::onSearchClicked() {
    loadData(searchEdit->text().trimmed().toStdString());
}

void DoctorView::onRefreshClicked() {
    searchEdit->clear();
    loadData();
}
