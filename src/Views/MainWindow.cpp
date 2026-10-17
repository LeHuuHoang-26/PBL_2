#include "MainWindow.h"
#include "DoctorView.h"
#include "PatientView.h"
#include "AppointmentView.h"
#include "StatisticView.h"
#include "AccountView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

// Hàm tạo nút menu sidebar đẹp mắt
static QPushButton* createMenuButton(const QString &text) {
    QPushButton *btn = new QPushButton(text);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(
        "QPushButton {"
        "   background-color: transparent;"
        "   color: #E2E8F0;"
        "   text-align: left;"
        "   padding: 12px 16px;"
        "   font-size: 14px;"
        "   border: none;"
        "   border-radius: 6px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #334155;"
        "   color: white;"
        "}"
        "QPushButton:checked {"
        "   background-color: #2563EB;"
        "   color: white;"
        "   font-weight: bold;"
        "}"
    );
    btn->setCheckable(true);
    return btn;
}

MainWindow::MainWindow(const std::string &userName, int role, QWidget *parent)
    : QMainWindow(parent) {
    setWindowTitle("MngCareBook - Hệ Thống Quản Lý Phòng Khám");
    resize(1200, 750);

    // --- 1. SIDEBAR BÊN TRÁI ---
    QFrame *sidebar = new QFrame(this);
    sidebar->setStyleSheet("background-color: #0F172A; min-width: 230px; max-width: 250px;");
    QVBoxLayout *sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(12, 20, 12, 20);
    sideLayout->setSpacing(8);

    // Logo / Brand title
    QLabel *brand = new QLabel("🏥 MngCareBook", sidebar);
    brand->setStyleSheet("color: white; font-size: 20px; font-weight: bold; padding-bottom: 15px; border-bottom: 1px solid #334155;");
    sideLayout->addWidget(brand);

    // Các nút menu
    btnDoctor = createMenuButton("👨‍⚕️ Quản lý Bác sĩ");
    btnPatient = createMenuButton("🧑‍🦽 Quản lý Bệnh nhân");
    btnAppointment = createMenuButton("📅 Đặt lịch khám");
    btnStatistic = createMenuButton("📊 Thống kê báo cáo");
    btnAccount = createMenuButton("🔐 Quản trị tài khoản");

    btnDoctor->setChecked(true); // Mặc định chọn trang Bác sĩ

    sideLayout->addWidget(btnDoctor);
    sideLayout->addWidget(btnPatient);
    sideLayout->addWidget(btnAppointment);
    sideLayout->addWidget(btnStatistic);

    // Chỉ Admin (Role == 3) mới hiển thị nút quản lý tài khoản
    if (role == 3) {
        sideLayout->addWidget(btnAccount);
    } else {
        btnAccount->hide();
    }

    sideLayout->addStretch();

    // Thông tin người đang đăng nhập ở góc dưới sidebar
    QString roleTitle = (role == 3) ? "Quản trị viên" : "Tiếp tân";
    userStatusLabel = new QLabel(QString("👤 %1\n(%2)").arg(QString::fromStdString(userName)).arg(roleTitle), sidebar);
    userStatusLabel->setStyleSheet("color: #94A3B8; font-size: 12px; padding-top: 10px; border-top: 1px solid #334155;");
    sideLayout->addWidget(userStatusLabel);

    // --- 2. VÙNG NỘI DUNG CHÍNH (STACKED WIDGET) ---
    stackedWidget = new QStackedWidget(this);
    stackedWidget->setStyleSheet("background-color: #F8FAFC; padding: 15px;");

    doctorView = new DoctorView(this);
    patientView = new PatientView(this);
    appointmentView = new AppointmentView(this);
    statisticView = new StatisticView(this);
    accountView = new AccountView(this);

    stackedWidget->addWidget(doctorView);      // Index 0
    stackedWidget->addWidget(patientView);     // Index 1
    stackedWidget->addWidget(appointmentView); // Index 2
    stackedWidget->addWidget(statisticView);   // Index 3
    stackedWidget->addWidget(accountView);     // Index 4

    // --- 3. GOM SIDEBAR VÀ NỘI DUNG VÀO CỬA SỔ CHÍNH ---
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *rootLayout = new QHBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(sidebar);
    rootLayout->addWidget(stackedWidget);
    setCentralWidget(centralWidget);

    // --- 4. KẾT NỐI TÍN HIỆU CHUYỂN TRANG ---
    connect(btnDoctor, &QPushButton::clicked, [this]() { switchPage(0); });
    connect(btnPatient, &QPushButton::clicked, [this]() { switchPage(1); });
    connect(btnAppointment, &QPushButton::clicked, [this]() { switchPage(2); });
    connect(btnStatistic, &QPushButton::clicked, [this]() { switchPage(3); });
    connect(btnAccount, &QPushButton::clicked, [this]() { switchPage(4); });
}

MainWindow::~MainWindow() {}

void MainWindow::switchPage(int index) {
    stackedWidget->setCurrentIndex(index);
    btnDoctor->setChecked(index == 0);
    btnPatient->setChecked(index == 1);
    btnAppointment->setChecked(index == 2);
    btnStatistic->setChecked(index == 3);
    btnAccount->setChecked(index == 4);
}
