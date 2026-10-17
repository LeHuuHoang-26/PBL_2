#include "StatisticView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>

// Hàm tiện ích tạo thẻ thống kê hình chữ nhật
static QFrame* createStatCard(const QString &title, QLabel *&valLabel, const QString &bgColor) {
    QFrame *card = new QFrame();
    card->setStyleSheet(QString(
        "QFrame { background-color: %1; border-radius: 10px; padding: 15px; color: white; }"
    ).arg(bgColor));

    QVBoxLayout *layout = new QVBoxLayout(card);
    QLabel *titleLbl = new QLabel(title, card);
    titleLbl->setStyleSheet("font-size: 14px; font-weight: bold; color: rgba(255,255,255,0.9);");

    valLabel = new QLabel("0", card);
    valLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: white;");

    layout->addWidget(titleLbl);
    layout->addWidget(valLabel);
    return card;
}

StatisticView::StatisticView(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Tiêu đề & nút refresh
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("<h2>📊 Báo Cáo & Thống Kê Phòng Khám</h2>", this);
    QPushButton *btnRefresh = new QPushButton("🔄 Cập nhật số liệu", this);

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(btnRefresh);
    mainLayout->addLayout(headerLayout);

    // Lưới thẻ thống kê (Grid layout)
    QGridLayout *gridLayout = new QGridLayout();

    QFrame *card1 = createStatCard("TỔNG SỐ BÁC SĨ", lblDoctorCount, "#2563EB");       // Xanh dương
    QFrame *card2 = createStatCard("TỔNG SỐ BỆNH NHÂN", lblPatientCount, "#059669");     // Xanh lá
    QFrame *card3 = createStatCard("TỔNG SỐ LỊCH HẸN", lblAppointmentCount, "#D97706");  // Cam
    QFrame *card4 = createStatCard("ĐÃ KHÁM XONG", lblCompletedCount, "#7C3AED");       // Tím

    gridLayout->addWidget(card1, 0, 0);
    gridLayout->addWidget(card2, 0, 1);
    gridLayout->addWidget(card3, 1, 0);
    gridLayout->addWidget(card4, 1, 1);

    mainLayout->addLayout(gridLayout);

    // Khu vực ghi chú hoặc tích hợp biểu đồ QChart sau này
    QLabel *chartPlaceholder = new QLabel(
        "<div style='text-align: center; padding: 40px; color: #64748B; background: #F1F5F9; border-radius: 8px;'>"
        "<h3>📈 Khu vực hiển thị biểu đồ thống kê (QtCharts)</h3>"
        "<p>Dữ liệu được tổng hợp trực tiếp từ StatisticController.</p>"
        "</div>", this);
    mainLayout->addWidget(chartPlaceholder);
    mainLayout->addStretch();

    connect(btnRefresh, &QPushButton::clicked, this, &StatisticView::onRefreshClicked);

    refreshStatistics();
}

void StatisticView::refreshStatistics() {
    // Gọi Controller thu thập dữ liệu thống kê tổng thể
    ClinicStatistics stats = controller.getClinicStatistics();

    lblDoctorCount->setText(QString::number(stats.totalDoctors));
    lblPatientCount->setText(QString::number(stats.totalPatients));
    lblAppointmentCount->setText(QString::number(stats.totalAppointments));
    lblCompletedCount->setText(QString::number(stats.completedAppointments));
}

void StatisticView::onRefreshClicked() {
    refreshStatistics();
}
