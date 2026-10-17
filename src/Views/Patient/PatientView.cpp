#include "PatientView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

PatientView::PatientView(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Tiêu đề
    QLabel *titleLabel = new QLabel("<h2>🧑‍🦽 Quản Lý Bệnh Nhân</h2>", this);
    mainLayout->addWidget(titleLabel);

    // Thanh tìm kiếm
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Tìm kiếm theo tên, mã bệnh nhân hoặc số CCCD...");
    btnSearch = new QPushButton("🔍 Tìm kiếm", this);
    btnRefresh = new QPushButton("🔄 Làm mới", this);

    searchLayout->addWidget(searchEdit, 1);
    searchLayout->addWidget(btnSearch);
    searchLayout->addWidget(btnRefresh);
    mainLayout->addLayout(searchLayout);

    // Bảng dữ liệu bệnh nhân
    tablePatients = new QTableWidget(this);
    tablePatients->setColumnCount(6);
    tablePatients->setHorizontalHeaderLabels({
        "Mã BN", "Họ và Tên", "Số ĐT", "Giới tính", "Tuổi", "Số CCCD"
    });
    tablePatients->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tablePatients->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tablePatients->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(tablePatients);

    // Dòng trạng thái
    statusLabel = new QLabel(this);
    mainLayout->addWidget(statusLabel);

    connect(btnSearch, &QPushButton::clicked, this, &PatientView::onSearchClicked);
    connect(btnRefresh, &QPushButton::clicked, this, &PatientView::onRefreshClicked);
    connect(searchEdit, &QLineEdit::returnPressed, this, &PatientView::onSearchClicked);

    loadData();
}

void PatientView::loadData(const std::string &keyword) {
    // Gọi Controller tìm kiếm và lấy danh sách bệnh nhân
    std::vector<Patient> patients = controller.searchPatients(keyword);
    int total = controller.getTotalPatientCount();

    tablePatients->setRowCount(0);
    for (size_t i = 0; i < patients.size(); ++i) {
        const Patient &p = patients[i];
        int row = (int)i;

        tablePatients->insertRow(row);
        tablePatients->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(p.getID())));
        tablePatients->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(p.getfullName())));
        tablePatients->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(p.getPhone())));
        tablePatients->setItem(row, 3, new QTableWidgetItem(p.getGender() ? "Nam" : "Nữ"));
        tablePatients->setItem(row, 4, new QTableWidgetItem(QString::number(p.getAge())));
        tablePatients->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(p.getCCCD())));
    }

    statusLabel->setText(QString("Hiển thị %1 / %2 bệnh nhân.").arg(patients.size()).arg(total));
}

void PatientView::onSearchClicked() {
    loadData(searchEdit->text().trimmed().toStdString());
}

void PatientView::onRefreshClicked() {
    searchEdit->clear();
    loadData();
}
