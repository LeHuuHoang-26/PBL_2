#include "AppointmentView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

AppointmentView::AppointmentView(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Tiêu đề
    QLabel *titleLabel = new QLabel("<h2>📅 Quản Lý Lịch Hẹn Khám Bệnh</h2>", this);
    mainLayout->addWidget(titleLabel);

    // Thanh tìm kiếm
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Tìm kiếm theo mã hẹn, mã bệnh nhân hoặc mã bác sĩ...");
    btnSearch = new QPushButton("🔍 Tìm kiếm", this);
    btnRefresh = new QPushButton("🔄 Làm mới", this);

    searchLayout->addWidget(searchEdit, 1);
    searchLayout->addWidget(btnSearch);
    searchLayout->addWidget(btnRefresh);
    mainLayout->addLayout(searchLayout);

    // Bảng dữ liệu lịch hẹn
    tableAppointments = new QTableWidget(this);
    tableAppointments->setColumnCount(6);
    tableAppointments->setHorizontalHeaderLabels({
        "Mã Hẹn", "Mã BN", "Mã BS", "Ngày Đặt", "Ngày Khám", "Trạng Thái"
    });
    tableAppointments->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableAppointments->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableAppointments->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(tableAppointments);

    // Dòng trạng thái
    statusLabel = new QLabel(this);
    mainLayout->addWidget(statusLabel);

    connect(btnSearch, &QPushButton::clicked, this, &AppointmentView::onSearchClicked);
    connect(btnRefresh, &QPushButton::clicked, this, &AppointmentView::onRefreshClicked);
    connect(searchEdit, &QLineEdit::returnPressed, this, &AppointmentView::onSearchClicked);

    loadData();
}

void AppointmentView::loadData(const std::string &keyword) {
    // Gọi Controller tìm kiếm và lấy dữ liệu
    std::vector<Appointment> apps = controller.searchAppointments(keyword);
    int total = controller.getTotalAppointmentCount();

    tableAppointments->setRowCount(0);
    for (size_t i = 0; i < apps.size(); ++i) {
        const Appointment &a = apps[i];
        int row = (int)i;

        QString stateStr = "Chờ khám";
        if (a.getState() == 1) stateStr = "Đã khám";
        else if (a.getState() == 2) stateStr = "Đã hủy";

        tableAppointments->insertRow(row);
        tableAppointments->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(a.getId())));
        tableAppointments->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(a.getPatientId())));
        tableAppointments->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(a.getDoctorId())));
        tableAppointments->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(a.getDateSet().toString())));
        tableAppointments->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(a.getDateGet().toString())));
        tableAppointments->setItem(row, 5, new QTableWidgetItem(stateStr));
    }

    statusLabel->setText(QString("Hiển thị %1 / %2 lịch hẹn.").arg(apps.size()).arg(total));
}

void AppointmentView::onSearchClicked() {
    loadData(searchEdit->text().trimmed().toStdString());
}

void AppointmentView::onRefreshClicked() {
    searchEdit->clear();
    loadData();
}
