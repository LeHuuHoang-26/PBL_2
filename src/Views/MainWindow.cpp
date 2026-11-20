#include "MainWindow.h"
#include "AppointmentView.h"
#include "PatientView.h"
#include "DoctorView.h"
#include "AccountView.h"
#include "StatisticView.h"
#include "Login.h"
#include "UIHelper.h"
#include "AppConstants.h"
#include "AppExceptions.h"

#include "AppointmentController.h"
#include "DoctorController.h"
#include "PatientController.h"
#include "ManagerController.h"
#include "AuthController.h"
#include "libPBL2.h"

#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QPointer>
#include <QStringList>
#include <QWidget>
#include <QStackedWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QIcon>
#include <QTimer>

using namespace std;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setCursor(Qt::ArrowCursor);
    setStyleSheet(appStyleSheet());
    setMinimumSize(1120, 720);
    resize(1360, 850);
    setWindowTitle("An Tâm · Quản lý phòng khám");

    const QStringList iconCandidates = {
        QStringLiteral(":/assets/textures/icon_app.ico"),
        QStringLiteral(":/assets/textures/icon_app.png"),
        QStringLiteral("assets/textures/icon_app.ico"),
        QStringLiteral("assets/textures/icon_app.png")
    };
    for (const QString &candidate : iconCandidates) {
        if (QFile::exists(candidate)) {
            QIcon icon(candidate);
            if (!icon.isNull()) {
                setWindowIcon(icon);
                break;
            }
        }
    }

    appInitialized = initApp();
}

void MainWindow::clearDataLists()
{
    delete currentManager;
    currentManager = nullptr;
    currentManagerId.clear();

    delete managers;
    managers = nullptr;
    delete patients;
    patients = nullptr;
    delete doctors;
    doctors = nullptr;
    delete symptoms;
    symptoms = nullptr;
    delete symptomPatients;
    symptomPatients = nullptr;
    delete symptomPatientDetails;
    symptomPatientDetails = nullptr;
    delete appointments;
    appointments = nullptr;
    delete permissionRecords;
    permissionRecords = nullptr;
}

void MainWindow::teardownShell()
{
    if (clockTimer) {
        clockTimer->stop();
        clockTimer = nullptr;
    }
    sidebarAvatarLabel = nullptr;
    sidebarNameLabel = nullptr;
    sidebarRoleLabel = nullptr;
    navButtons.clear();
    patientView = nullptr;
    doctorView = nullptr;
    accountView = nullptr;
    statisticView = nullptr;
    sidebar = nullptr;
    pages = nullptr;
    activeNav = 0;

    if (root) {
        root->hide();
        if (windowStack) {
            windowStack->removeWidget(root);
        }
        delete root;
        root = nullptr;
    }
    if (appointmentView) {
        delete appointmentView;
        appointmentView = nullptr;
    }
}

MainWindow::~MainWindow()
{
    teardownShell();
    clearDataLists();
}

bool MainWindow::initApp()
{
    teardownShell();
    clearDataLists();

    DoctorController::clearSpecialtyCache();
    AppointmentController::clearHolidayCache();

    const QString permFilePath = QString::fromUtf8(AppConstants::Paths::PERMISSION_MANAGER);
    if (!QFile::exists(permFilePath)) {
        try {
            permissionRecords = new LinkList<PermissionManager>;
        } catch (const MemoryAllocationException &e) {
            UIHelper::showWarning(this, "Lỗi bộ nhớ", QString::fromUtf8(e.what()));
            return false;
        } catch (const std::exception &e) {
            UIHelper::showWarning(this, "Lỗi hệ thống", QString("Không thể khởi tạo danh sách phân quyền: %1").arg(e.what()));
            return false;
        }
    } else {
        try {
            permissionRecords = PermissionManager::loadFromFile(AppConstants::Paths::PERMISSION_MANAGER);
        } catch (const FileIOException &e) {
            UIHelper::showWarning(this, "Lỗi tệp phân quyền",
                QString::fromUtf8(e.what()) + "\nKhông thể mở tệp phân quyền hiện có. Vui lòng kiểm tra quyền đọc/ghi.");
            return false;
        } catch (const std::exception &e) {
            UIHelper::showWarning(this, "Lỗi phân quyền",
                QString("Không thể nạp cấu hình phân quyền: %1").arg(e.what()));
            return false;
        }
    }

    if (ManagerController::cleanExpiredPermissions(permissionRecords)) {
        try {
            PermissionManager::saveToFile(AppConstants::Paths::PERMISSION_MANAGER, permissionRecords);
        } catch (const std::exception &e) {
            UIHelper::showWarning(this, "Lỗi cập nhật quyền hạn", QString("Không thể lưu lại bảng phân quyền sau khi dọn dẹp: %1").arg(e.what()));
        }
    }

    try {
        managers = Manager::loadFromFile(AppConstants::Paths::MANAGER);
        patients = Patient::loadFromFile(AppConstants::Paths::PATIENT);
        doctors = Doctor::loadFromFile(AppConstants::Paths::DOCTOR);
        symptoms = Symptom::loadFromFile(AppConstants::Paths::SYMPTOM);
        symptomPatients = SymptomPatient::loadFromFile(AppConstants::Paths::SYMPTOM_PATIENT);
        symptomPatientDetails = SymptomPatientDetail::loadFromFile(AppConstants::Paths::SYMPTOM_PATIENT_DETAIL);
        appointments = Appointment::loadFromFile(AppConstants::Paths::APPOINTMENT);
        if (appointments) {
            int synced = AppointmentController::syncExpiredAppointments(appointments);
            if (synced > 0) {
                try {
                    Appointment::saveToFile(AppConstants::Paths::APPOINTMENT, appointments);
                } catch (const std::exception &e) {
                    UIHelper::showWarning(this, "Cảnh báo đồng bộ lịch hẹn",
                        QString("Đã cập nhật %1 lịch trễ hẹn trong bộ nhớ nhưng chưa ghi được xuống tệp: %2")
                            .arg(synced).arg(QString::fromUtf8(e.what())));
                }
            }
        }
    } catch (const FileIOException &e) {
        UIHelper::showWarning(this, "Lỗi tệp dữ liệu", QString::fromUtf8(e.what()) + "\nVui lòng kiểm tra lại thư mục Data/.");
        return false;
    } catch (const DataFormatException &e) {
        UIHelper::showWarning(this, "Lỗi định dạng dữ liệu", QString::fromUtf8(e.what()));
        return false;
    } catch (const MemoryAllocationException &e) {
        UIHelper::showWarning(this, "Lỗi bộ nhớ", QString::fromUtf8(e.what()));
        return false;
    } catch (const AppException &e) {
        UIHelper::showWarning(this, "Lỗi nạp dữ liệu", QString::fromUtf8(e.what()));
        return false;
    } catch (const std::exception &e) {
        UIHelper::showWarning(this, "Lỗi ngoại lệ hệ thống", QString("Đã xảy ra ngoại lệ: %1").arg(e.what()));
        return false;
    }

    bool ok = managers && patients && doctors && symptoms && symptomPatients && symptomPatientDetails && appointments;
    if (!ok)
    {
        UIHelper::showWarning(this, "Lỗi dữ liệu", "Không thể nạp dữ liệu từ thư mục Data/! Vui lòng đảm bảo tệp dữ liệu tồn tại và ứng dụng được khởi chạy từ đúng thư mục.");
        return false;
    }

    if (patients) PatientController::sortPatients(patients, PatientSortBy::ID, false);
    if (doctors) DoctorController::sortDoctors(doctors, DoctorSortBy::ID, false);
    if (appointments) AppointmentController::sortAppointmentsById(appointments, false);
    if (managers) ManagerController::sortManagers(managers, AccountSortBy::ID, false, permissionRecords);
    if (symptomPatients) {
        libPBL2::qsort(symptomPatients, [](const SymptomPatient &a, const SymptomPatient &b) {
            return libPBL2::compareNatural(a.getSP_id(), b.getSP_id()) < 0;
        });
    }
    if (symptomPatientDetails) {
        libPBL2::qsort(symptomPatientDetails, [](const SymptomPatientDetail &a, const SymptomPatientDetail &b) {
            int cmp = libPBL2::compareNatural(a.getSP_id(), b.getSP_id());
            if (cmp != 0) return cmp < 0;
            return libPBL2::compareNatural(a.getS_id(), b.getS_id()) < 0;
        });
    }

    if (!windowStack) {
        windowStack = new QStackedWidget(this);
        setCentralWidget(windowStack);
    }
    if (!loginView) {
        loginView = new Login(windowStack);
        windowStack->addWidget(loginView);
        connect(loginView, &Login::loginClicked, this, &MainWindow::handleLogin);
    }

    showLogin();
    return true;
}

void MainWindow::showLogin()
{
    setWindowTitle("An Tâm · Đăng nhập");
    if (windowStack && loginView)
    {
        windowStack->setCurrentWidget(loginView);
    }
    if (loginView)
    {
        loginView->clearInputs();
        loginView->clearMessage();
    }
}

void MainWindow::handleLogin()
{
    if (!loginView) return;
    loginView->clearMessage();

    QString username = loginView->getUsername();
    QString password = loginView->getPassword();

    QString errorMessage;
    Manager *manager = nullptr;
    bool success = AuthController::login(managers, username, password, errorMessage, manager);

    if (success && manager)
    {
        delete currentManager;
        currentManager = new Manager(*manager);
        currentManagerId = currentManager->getID();
        m_startedAsTempAdmin = ManagerController::isTemporaryAdmin(currentManager, permissionRecords);
        m_forceLogoutInProgress = false;

        setupShell();
        setWindowTitle("An Tâm · Quản lý phòng khám");
    }
    else
    {
        loginView->showErrorMessage(errorMessage);
    }
}

void MainWindow::handleLogout()
{
    if (!UIHelper::confirmDelete(
            this,
            QStringLiteral("Xác nhận đăng xuất"),
            QStringLiteral("Bạn có chắc chắn muốn đăng xuất khỏi hệ thống không?\nDữ liệu phiên làm việc sẽ được lưu tự động."),
            QStringLiteral("Đăng xuất"),
            QStringLiteral("Hủy bỏ")))
    {
        return;
    }

    if (!saveAll(SaveScope::All, SaveAction::CloseOrLogout)) {
        if (!UIHelper::confirmWarning(
                this,
                QStringLiteral("Lưu dữ liệu chưa hoàn tất"),
                QStringLiteral("Không thể lưu toàn bộ dữ liệu trước khi đăng xuất. Bạn vẫn muốn thoát phiên làm việc này?"),
                QStringLiteral("Vẫn đăng xuất"),
                QStringLiteral("Ở lại"))) {
            return;
        }
    }

    teardownShell();
    delete currentManager;
    currentManager = nullptr;
    currentManagerId.clear();
    m_startedAsTempAdmin = false;
    m_forceLogoutInProgress = false;
    showLogin();
}

void MainWindow::setPage(int index)
{
    if (!pages || index < 0 || index >= pages->count())
        return;

    if (index == 5 && !isAdministrator())
    {
        UIHelper::showWarning(this, QStringLiteral("Không có quyền truy cập"),
            QStringLiteral("Chỉ Quản trị viên mới có quyền xem mục Thống kê và Doanh thu phòng khám."));
        return;
    }

    if (index != activeNav)
    {
        resetTabOrderToId(activeNav);
    }
    activeNav = index;
    pages->setCurrentIndex(index);
    updateNavigation();
}

void MainWindow::refreshPage(int index)
{
    if (!pages || index < 0 || index >= pages->count())
        return;

    bool wasCurrent = (pages->currentIndex() == index);

    switch (index)
    {
    case 0:
        if (appointmentView)
        {
            QWidget *old = pages->widget(0);
            QWidget *fresh = appointmentView->createDashboardWidget();
            pages->insertWidget(0, fresh);
            if (wasCurrent) pages->setCurrentIndex(0);
            if (old) {
                old->hide();
                pages->removeWidget(old);
                old->deleteLater();
            }
        }
        break;
    case 1:
        if (appointmentView)
        {
            QWidget *old = pages->widget(1);
            QWidget *fresh = appointmentView->createBookingWidget();
            pages->insertWidget(1, fresh);
            if (wasCurrent) pages->setCurrentIndex(1);
            if (old) {
                old->hide();
                pages->removeWidget(old);
                old->deleteLater();
            }
        }
        break;
    case 2:
        if (patientView)
            patientView->resetToFirstPage();
        break;
    case 3:
        if (doctorView)
            doctorView->resetToFirstPage();
        break;
    case 4:
        if (appointmentView)
        {
            appointmentView->resetToFirstPage();
            QWidget *old = pages->widget(4);
            QWidget *fresh = appointmentView->createAppointmentListWidget();
            pages->insertWidget(4, fresh);
            if (wasCurrent) pages->setCurrentIndex(4);
            if (old) {
                old->hide();
                pages->removeWidget(old);
                old->deleteLater();
            }
        }
        break;
    case 5:
        if (statisticView && isAdministrator())
            statisticView->refresh();
        break;
    case 6:
        if (accountView)
            accountView->resetToFirstPage();
        break;
    }
}

bool MainWindow::saveAll(SaveScope scope, SaveAction action)
{
    bool ok = true;
    bool warned = false;

    auto saveSymptoms = [&]() {
        if (symptomPatients)
        {
            libPBL2::qsort(symptomPatients, [](const SymptomPatient &a, const SymptomPatient &b) {
                return libPBL2::compareNatural(a.getSP_id(), b.getSP_id()) < 0;
            });
            ok = SymptomPatient::saveToFile(AppConstants::Paths::SYMPTOM_PATIENT, symptomPatients) && ok;
        }
        if (symptomPatientDetails)
        {
            libPBL2::qsort(symptomPatientDetails, [](const SymptomPatientDetail &a, const SymptomPatientDetail &b) {
                int cmp = libPBL2::compareNatural(a.getSP_id(), b.getSP_id());
                if (cmp != 0) return cmp < 0;
                return libPBL2::compareNatural(a.getS_id(), b.getS_id()) < 0;
            });
            ok = SymptomPatientDetail::saveToFile(AppConstants::Paths::SYMPTOM_PATIENT_DETAIL, symptomPatientDetails) && ok;
        }
    };

    try {
        if (scope == SaveScope::All || scope == SaveScope::Patient)
        {
            if (patients) PatientController::sortPatients(patients, PatientSortBy::ID, true);
            saveSymptoms();
            ok = Patient::saveToFile(AppConstants::Paths::PATIENT, patients) && ok;

            if (action == SaveAction::Normal && patientView)
            {
                patientView->reapplyCurrentSort();
            }
        }

        if (scope == SaveScope::All || scope == SaveScope::Doctor)
        {
            if (doctors) DoctorController::sortDoctors(doctors, DoctorSortBy::ID, true);
            ok = Doctor::saveToFile(AppConstants::Paths::DOCTOR, doctors) && ok;

            if (action == SaveAction::Normal && doctorView)
            {
                doctorView->reapplyCurrentSort();
            }
        }

        if (scope == SaveScope::All || scope == SaveScope::Appointment)
        {
            if (appointments) {
                AppointmentController::syncExpiredAppointments(appointments);
                AppointmentController::sortAppointmentsById(appointments, true);
            }
            if (scope == SaveScope::Appointment)
            {
                saveSymptoms();
            }
            ok = Appointment::saveToFile(AppConstants::Paths::APPOINTMENT, appointments) && ok;
        }

        if (scope == SaveScope::All || scope == SaveScope::Account)
        {
            if (managers) ManagerController::sortManagers(managers, AccountSortBy::ID, true, permissionRecords);
            ok = Manager::saveToFile(AppConstants::Paths::MANAGER, managers) && ok;

            if (permissionRecords) {
                ok = PermissionManager::saveToFile(AppConstants::Paths::PERMISSION_MANAGER, permissionRecords) && ok;
            }

            if (action == SaveAction::Normal && accountView)
            {
                accountView->reapplyCurrentSort();
            }
        }
    } catch (const FileIOException &e) {
        ok = false;
        warned = true;
        UIHelper::showWarning(this, "Lỗi ghi tệp dữ liệu", QString::fromUtf8(e.what()));
    } catch (const AppException &e) {
        ok = false;
        warned = true;
        UIHelper::showWarning(this, "Lỗi hệ thống", QString::fromUtf8(e.what()));
    } catch (const std::exception &e) {
        ok = false;
        warned = true;
        UIHelper::showWarning(this, "Lỗi ngoại lệ", QString("Đã xảy ra ngoại lệ khi lưu: %1").arg(e.what()));
    }

    if (!ok && !warned)
    {
        UIHelper::showWarning(this, "Lỗi lưu dữ liệu", "Không thể lưu một hoặc nhiều tệp dữ liệu.");
    }
    return ok;
}

Manager *MainWindow::getCurrentManager() const
{
    return currentManager;
}

void MainWindow::resetTabOrderToId(int tabIndex)
{
    switch (tabIndex)
    {
    case 2:
        if (patients)
        {
            PatientController::sortPatients(patients, PatientSortBy::ID, false);
        }
        break;
    case 3:
        if (doctors)
        {
            DoctorController::sortDoctors(doctors, DoctorSortBy::ID, false);
        }
        break;
    case 4:
        if (appointmentView)
        {
            appointmentView->resetToFirstPage();
        }
        else if (appointments)
        {
            AppointmentController::sortAppointmentsById(appointments, false);
        }
        break;
    case 6:
        if (managers)
        {
            ManagerController::sortManagers(managers, AccountSortBy::ID, false, permissionRecords);
        }
        break;
    default:
        break;
    }
}

void MainWindow::updateUserProfileDisplay()
{
    Manager *curM = getCurrentManager();
    QString currentName = curM ? UIHelper::qs(curM->getfullName()) : "Quản trị viên";
    if (sidebarAvatarLabel)
    {
        sidebarAvatarLabel->setPixmap(UIHelper::createAvatar(currentName, 38));
    }
    if (sidebarNameLabel)
    {
        sidebarNameLabel->setText(currentName);
    }
    if (sidebarRoleLabel)
    {
        sidebarRoleLabel->setText(curM ? ManagerController::displayRoleName(curM, permissionRecords) : "Quản trị hệ thống");
    }
    if (navButtons.size() > 5 && navButtons[5])
    {
        navButtons[5]->setVisible(isAdministrator());
    }
}

bool MainWindow::isRootAdmin() const
{
    return ManagerController::isRootAdmin(getCurrentManager());
}

bool MainWindow::isAdministrator() const
{
    return ManagerController::isAdministrator(getCurrentManager(), permissionRecords);
}

bool MainWindow::hasPermission(const string &permissionId) const
{
    return ManagerController::hasPermission(getCurrentManager(), permissionId, permissionRecords);
}

QString MainWindow::specialty(const string &id) const
{
    return DoctorController::getSpecialty(id, AppConstants::Paths::SPECIALIZATION);
}

Patient *MainWindow::findPatient(const string &id) const
{
    return PatientController::findById(patients, id);
}

Doctor *MainWindow::findDoctor(const string &id) const
{
    return DoctorController::findById(doctors, id);
}

SymptomPatient *MainWindow::findSymptomPatient(const string &id) const
{
    if (!symptomPatients || id.empty()) return nullptr;
    auto *node = symptomPatients->getHeader()->next;
    while (node && node->next) {
        if (node->info.getSP_id() == id) {
            return &node->info;
        }
        node = node->next;
    }
    return nullptr;
}

Patient *MainWindow::findPatientForAppointment(const Appointment &a) const
{
    SymptomPatient *sp = findSymptomPatient(a.getSymptomPatientId());
    if (sp) {
        return findPatient(sp->getP_id());
    }
    return nullptr;
}

string MainWindow::getPatientIdForAppointment(const Appointment &a) const
{
    SymptomPatient *sp = findSymptomPatient(a.getSymptomPatientId());
    if (sp) {
        return sp->getP_id();
    }
    return "";
}

QString MainWindow::getSymptomNamesForAppointment(const Appointment &a) const
{
    SymptomPatient *sp = findSymptomPatient(a.getSymptomPatientId());
    if (!sp) return QStringLiteral("Chưa ghi nhận");

    QStringList names;
    if (symptomPatientDetails && symptoms) {
        auto *spdNode = symptomPatientDetails->getHeader()->next;
        while (spdNode && spdNode->next) {
            if (spdNode->info.getSP_id() == sp->getSP_id()) {
                string sId = spdNode->info.getS_id();
                auto *sNode = symptoms->getHeader()->next;
                while (sNode && sNode->next) {
                    if (sNode->info.getS_id() == sId) {
                        names.append(QString::fromUtf8(sNode->info.getSName().c_str()));
                        break;
                    }
                    sNode = sNode->next;
                }
            }
            spdNode = spdNode->next;
        }
    }
    names.removeDuplicates();
    return names.isEmpty() ? QStringLiteral("Chưa ghi nhận") : names.join(QStringLiteral(", "));
}

Date MainWindow::getDateSetForAppointment(const Appointment &a) const
{
    SymptomPatient *sp = findSymptomPatient(a.getSymptomPatientId());
    return sp ? sp->getDate() : Date();
}

bool MainWindow::isHoliday(const QDate &date) const
{
    return AppointmentController::isHoliday(date, AppConstants::Paths::HOLIDAYS);
}

QString MainWindow::holidayNote(const QDate &date) const
{
    return AppointmentController::getHolidayNote(date, AppConstants::Paths::HOLIDAYS);
}

string MainWindow::nextAppointmentId() const
{
    return AppointmentController::generateNextId(appointments);
}

bool MainWindow::showBookingDialog(const QDate &defaultDate, const std::string &preselectedPatientId)
{
    return appointmentView ? appointmentView->showBookingDialog(defaultDate, preselectedPatientId) : false;
}

void MainWindow::editAppointmentDialog(const string &id)
{
    if (appointmentView) {
        appointmentView->editAppointmentDialog(id);
    }
}

void MainWindow::showRescheduledSummaryDialog(QWidget *parent, const QList<RescheduledResultItem> &items, int totalAffected, const QString &customTitle)
{
    if (appointmentView) {
        appointmentView->showRescheduledSummaryDialog(parent, items, totalAffected, customTitle);
    }
}

QString MainWindow::appStyleSheet() const
{
    return QStringLiteral(R"(
            QWidget { font-family: 'Be Vietnam Pro', 'Segoe UI', sans-serif; font-size: 14px; color: #183044; }
            QMainWindow, QWidget#appRoot { background: #f4f7fb; }
            QLabel#pageTitle { font-size: 26px; font-weight: 700; color: #17334a; }
            QLabel#pageSubtitle { font-size: 13.5px; color: #64748b; }
            QFrame#sidebar { background: #102f43; border: 0; }
            QLabel#brand { color: white; font-size: 20px; font-weight: 700; letter-spacing: 0.5px; }
            QLabel#brandSub { color: #9db7c6; font-size: 11px; font-weight: 600; letter-spacing: 1px; }
            QPushButton#nav { color: #bfd0d9; text-align: left; padding: 12px 16px; border: 0; border-radius: 9px; background: transparent; font-size: 14px; font-weight: 500; }
            QPushButton#nav:hover { color: white; background: #1b465d; }
            QPushButton#nav[active="true"] { color: white; background: #167d79; font-weight: 600; }
            QPushButton#primary { background: #167d79; color: white; border: 0; border-radius: 9px; padding: 11px 20px; font-weight: 600; font-size: 14px; }
            QPushButton#primary:hover { background: #106d69; }
            QPushButton#secondary { background: white; color: #355166; border: 1px solid #dce5eb; border-radius: 9px; padding: 10px 16px; font-weight: 500; }
            QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QDateTimeEdit, QDateEdit { background: white; border: 1px solid #d8e2e9; border-radius: 8px; padding: 9px 12px; min-height: 20px; selection-background-color: #167d79; }
            QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus, QDateTimeEdit:focus, QDateEdit:focus { border: 1px solid #167d79; }
            QComboBox, QDateEdit, QDateTimeEdit {
                padding-right: 30px;
            }
            QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled, QComboBox:disabled, QDateEdit:disabled, QDateTimeEdit:disabled {
                background: #e2e8f0;
                color: #475569;
                border-color: #cbd5e1;
            }
            QComboBox::drop-down, QDateEdit::drop-down, QDateTimeEdit::drop-down {
                subcontrol-origin: border;
                subcontrol-position: top right;
                width: 28px;
                border: none;
                background: transparent;
            }
            QComboBox::drop-down:disabled, QDateEdit::drop-down:disabled, QDateTimeEdit::drop-down:disabled {
                width: 0px;
                border: none;
            }
            QComboBox::down-arrow, QDateEdit::down-arrow, QDateTimeEdit::down-arrow {
                image: url(:/assets/textures/arrow_down_dark.png);
                width: 11px;
                height: 11px;
                margin-right: 8px;
            }
            QComboBox::down-arrow:disabled, QDateEdit::down-arrow:disabled, QDateTimeEdit::down-arrow:disabled {
                image: none;
            }
            QSpinBox, QDoubleSpinBox {
                padding-right: 30px;
            }
            QSpinBox:disabled, QDoubleSpinBox:disabled {
                padding-right: 12px;
            }
            QSpinBox::up-button:disabled, QDoubleSpinBox::up-button:disabled,
            QSpinBox::down-button:disabled, QDoubleSpinBox::down-button:disabled {
                width: 0px;
                border: none;
                background: transparent;
            }
            QSpinBox::up-arrow:disabled, QDoubleSpinBox::up-arrow:disabled,
            QSpinBox::down-arrow:disabled, QDoubleSpinBox::down-arrow:disabled {
                image: none;
            }
            QSpinBox::up-button, QDoubleSpinBox::up-button {
                subcontrol-origin: border;
                subcontrol-position: top right;
                width: 25px;
                border-left: 1.5px solid #94a3b8;
                border-bottom: 1px solid #94a3b8;
                border-top-right-radius: 7px;
                background: #e2e8f0;
            }
            QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover {
                background: #0d9488;
            }
            QSpinBox::up-button:pressed, QDoubleSpinBox::up-button:pressed {
                background: #0f766e;
            }
            QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {
                image: url(:/assets/textures/arrow_up_dark.png);
                width: 9px;
                height: 9px;
            }
            QSpinBox::up-arrow:hover, QDoubleSpinBox::up-arrow:hover {
                image: url(:/assets/textures/arrow_up_white.png);
            }
            QSpinBox::down-button, QDoubleSpinBox::down-button {
                subcontrol-origin: border;
                subcontrol-position: bottom right;
                width: 25px;
                border-left: 1.5px solid #94a3b8;
                border-bottom-right-radius: 7px;
                background: #e2e8f0;
            }
            QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
                background: #0d9488;
            }
            QSpinBox::down-button:pressed, QDoubleSpinBox::down-button:pressed {
                background: #0f766e;
            }
            QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {
                image: url(:/assets/textures/arrow_down_dark.png);
                width: 9px;
                height: 9px;
            }
            QSpinBox::down-arrow:hover, QDoubleSpinBox::down-arrow:hover {
                image: url(:/assets/textures/arrow_down_white.png);
            }
            QComboBox QAbstractItemView {
                background-color: #ffffff;
                color: #0f172a;
                border: 1.5px solid #cbd5e1;
                border-radius: 8px;
                padding: 4px;
                selection-background-color: #0d9488;
                selection-color: #ffffff;
                outline: none;
            }
            QComboBox QAbstractItemView::item {
                min-height: 28px;
                padding: 4px 10px;
                border-radius: 6px;
                color: #0f172a;
                background-color: #ffffff;
            }
            QComboBox QAbstractItemView::item:hover {
                background-color: #f0fdfa;
                color: #0f766e;
            }
            QComboBox QAbstractItemView::item:selected {
                background-color: #0d9488;
                color: #ffffff;
            }
            QScrollBar:vertical, QScrollBar:horizontal {
                border: none;
                background: transparent;
                width: 0px;
                height: 0px;
                margin: 0px;
            }
            QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
                background: transparent;
                border: none;
                min-height: 0px;
                min-width: 0px;
            }
            QScrollBar::add-line, QScrollBar::sub-line,
            QScrollBar::add-page, QScrollBar::sub-page {
                background: transparent;
                border: none;
                width: 0px;
                height: 0px;
            }
            QTableWidget { background: white; border: 1px solid #e2e8f0; border-radius: 12px; gridline-color: transparent; selection-background-color: #f1f5f9; selection-color: #17334a; }
            QHeaderView { border: none; background: transparent; }
            QHeaderView::section { background: #f8fafc; color: #475569; border: none; border-bottom: 2px solid #e2e8f0; border-left: none; border-right: none; padding: 12px 20px; font-size: 12px; font-weight: 700; letter-spacing: 0.5px; }
            QTableWidget::item { padding: 4px 8px; font-size: 13.5px; color: #1e293b; border: none; border-bottom: 1px solid #f1f5f9; border-left: none; border-right: none; }
            QTableWidget::item:hover { background-color: #f8fafc; }
            QFrame#card, QFrame#panel { background: white; border: 1px solid #e2e8f0; border-radius: 14px; }
            QCheckBox { spacing: 9px; }
        )");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (clockTimer) {
        clockTimer->stop();
    }
    if (currentManager != nullptr)
    {
        saveAll(SaveScope::All, SaveAction::CloseOrLogout);
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::updateNavigation()
{
    for (int i = 0; i < navButtons.size(); ++i)
    {
        bool wasActive = navButtons[i]->property("active").toBool();
        bool isActive = (i == activeNav);
        if (wasActive != isActive)
        {
            navButtons[i]->setProperty("active", isActive);
            navButtons[i]->style()->unpolish(navButtons[i]);
            navButtons[i]->style()->polish(navButtons[i]);
        }
    }
}

void MainWindow::setupShell()
{
    teardownShell();

    root = new QWidget;
    root->setObjectName("appRoot");
    root->setCursor(Qt::ArrowCursor);

    auto *outer = new QHBoxLayout(root);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    sidebar = new QFrame;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(238);
    sidebar->setCursor(Qt::ArrowCursor);

    auto *side = new QVBoxLayout(sidebar);
    side->setContentsMargins(18, 22, 18, 16);
    side->setSpacing(8);

    auto *brand = new QLabel("✦  AN TÂM");
    brand->setObjectName("brand");
    auto *brandSub = new QLabel("PHÒNG KHÁM ĐA KHOA");
    brandSub->setObjectName("brandSub");
    side->addWidget(brand);
    side->addWidget(brandSub);
    side->addSpacing(27);

    navButtons.clear();
    const QStringList labels = {
        "⌂   Tổng quan",
        "＋   Đặt lịch khám",
        "♡   Bệnh nhân",
        "✚   Bác sĩ",
        "▤   Quản lý lịch khám",
        "📊   Thống kê",
        "♙   Quản lý tài khoản"};
    for (int i = 0; i < labels.size(); ++i)
    {
        auto *b = UIHelper::button(labels[i], "nav");
        navButtons.push_back(b);
        if (i == 5 && !isAdministrator()) {
            b->setVisible(false);
        }
        connect(b, &QPushButton::clicked, this, [this, i]
        {
            if (i != activeNav)
            {
                setPage(i);
            }
        });
        side->addWidget(b);
    }

    side->addStretch();

    QFrame *userCard = new QFrame;
    userCard->setObjectName("userCard");
    userCard->setStyleSheet(R"(
            QFrame#userCard {
                background: #14384e;
                border: 1px solid #1e4b66;
                border-radius: 12px;
            }
        )");
    auto *userLayout = new QVBoxLayout(userCard);
    userLayout->setContentsMargins(12, 10, 12, 10);
    userLayout->setSpacing(8);

    auto *profileRow = new QHBoxLayout;
    profileRow->setSpacing(10);

    sidebarAvatarLabel = new QLabel;
    sidebarAvatarLabel->setFixedSize(38, 38);
    profileRow->addWidget(sidebarAvatarLabel);

    auto *nameCol = new QVBoxLayout;
    nameCol->setSpacing(2);
    sidebarNameLabel = new QLabel;
    sidebarNameLabel->setStyleSheet("color: white; font-weight: 700; font-size: 13px;");
    sidebarNameLabel->setWordWrap(true);

    sidebarRoleLabel = new QLabel;
    sidebarRoleLabel->setStyleSheet("color: #8faec0; font-size: 11px; font-weight: 500;");

    nameCol->addWidget(sidebarNameLabel);
    nameCol->addWidget(sidebarRoleLabel);
    profileRow->addLayout(nameCol, 1);
    userLayout->addLayout(profileRow);
    updateUserProfileDisplay();

    QPushButton *logout = new QPushButton("↪   Đăng xuất");
    logout->setCursor(Qt::PointingHandCursor);
    logout->setStyleSheet(R"(
            QPushButton {
                background: rgba(239, 68, 68, 0.12);
                color: #fca5a5;
                border: 1px solid rgba(239, 68, 68, 0.35);
                border-radius: 8px;
                padding: 7px 12px;
                font-weight: 600;
                font-size: 12.5px;
            }
            QPushButton:hover {
                background: #dc2626;
                color: white;
                border-color: #dc2626;
            }
        )");
    connect(logout, &QPushButton::clicked, this, &MainWindow::handleLogout);
    userLayout->addWidget(logout);
    side->addWidget(userCard);

    side->addSpacing(8);

    QFrame *timeCard = new QFrame;
    timeCard->setObjectName("timeCard");
    timeCard->setStyleSheet(R"(
            QFrame#timeCard {
                background: #14384e;
                border: 1px solid #1e4b66;
                border-radius: 12px;
            }
        )");
    auto *timeLayout = new QVBoxLayout(timeCard);
    timeLayout->setContentsMargins(12, 8, 12, 8);
    timeLayout->setSpacing(2);

    auto *timeRow = new QHBoxLayout;
    timeRow->setSpacing(7);

    auto *clockDot = new QLabel("●");
    clockDot->setStyleSheet("color: #2dd4bf; font-size: 11px; background: transparent; border: none;");

    auto *timeLabel = new QLabel;
    timeLabel->setStyleSheet("color: #f8fafc; font-weight: 700; font-size: 14.5px; font-family: 'Consolas', 'Segoe UI', monospace; letter-spacing: 0.5px; background: transparent; border: none;");

    timeRow->addWidget(clockDot);
    timeRow->addWidget(timeLabel);
    timeRow->addStretch();

    auto *dateLabel = new QLabel;
    dateLabel->setStyleSheet("color: #8faec0; font-size: 11px; font-weight: 500; margin-left: 2px; background: transparent; border: none;");

    timeLayout->addLayout(timeRow);
    timeLayout->addWidget(dateLabel);

    auto updateClock = [timeLabel, dateLabel]() {
        QDateTime now = QDateTime::currentDateTime();
        timeLabel->setText(now.toString("HH:mm:ss"));
        static const QString dowNames[] = {
            "", "Thứ Hai", "Thứ Ba", "Thứ Tư", "Thứ Năm", "Thứ Sáu", "Thứ Bảy", "Chủ Nhật"
        };
        int dow = now.date().dayOfWeek();
        QString dowStr = (dow >= 1 && dow <= 7) ? dowNames[dow] : "";
        dateLabel->setText(QStringLiteral("%1, %2").arg(dowStr, now.toString("dd/MM/yyyy")));
    };

    clockTimer = new QTimer(timeCard);
    connect(clockTimer, &QTimer::timeout, timeCard, [this, updateClock]() {
        updateClock();
        if (m_startedAsTempAdmin && !m_forceLogoutInProgress &&
            !ManagerController::isTemporaryAdmin(getCurrentManager(), permissionRecords)) {
            handleTemporaryAdminExpired();
        }
    });
    clockTimer->start(1000);
    updateClock();

    side->addWidget(timeCard);

    pages = new QStackedWidget;
    pages->setCursor(Qt::ArrowCursor);
    outer->addWidget(sidebar);
    outer->addWidget(pages, 1);

    appointmentView = new AppointmentView(this, this);

    pages->addWidget(appointmentView->createDashboardWidget());

    pages->addWidget(appointmentView->createBookingWidget());

    patientView = new PatientView(this);
    pages->addWidget(patientView);

    doctorView = new DoctorView(this);
    pages->addWidget(doctorView);

    pages->addWidget(appointmentView->createAppointmentListWidget());

    statisticView = new StatisticView(this);
    pages->addWidget(statisticView);

    accountView = new AccountView(this);
    pages->addWidget(accountView);

    if (windowStack)
    {
        windowStack->addWidget(root);
        windowStack->setCurrentWidget(root);
    }

    setPage(0);
}

void MainWindow::handleTemporaryAdminExpired()
{
    if (m_forceLogoutInProgress) return;
    m_forceLogoutInProgress = true;
    if (clockTimer) {
        clockTimer->stop();
    }

    QList<QPointer<QDialog>> openDialogs;
    const auto topWidgets = QApplication::topLevelWidgets();
    for (QWidget *w : topWidgets) {
        if (auto *dlg = qobject_cast<QDialog*>(w)) {
            if (dlg->isVisible()) {
                openDialogs.append(dlg);
            }
        }
    }
    for (const QPointer<QDialog> &dlg : openDialogs) {
        if (dlg && dlg->isVisible()) {
            dlg->reject();
        }
    }

    QTimer::singleShot(0, this, [this]() {
        if (permissionRecords) {
            ManagerController::cleanExpiredPermissions(permissionRecords);
        }
        bool savedOk = saveAll(SaveScope::All, SaveAction::CloseOrLogout);

        QString msg = savedOk
            ? QStringLiteral("Quyền Quản trị viên tạm thời (Per_1) của bạn đã hết hạn.\n"
                             "Toàn bộ dữ liệu đã được lưu an toàn. Hệ thống sẽ đăng xuất để cập nhật lại quyền hạn.")
            : QStringLiteral("Quyền Quản trị viên tạm thời (Per_1) của bạn đã hết hạn.\n"
                             "Lưu ý: Một số dữ liệu chưa thể lưu xuống tệp. Hệ thống sẽ đăng xuất để cập nhật lại quyền hạn.");

        UIHelper::showWarning(
            this,
            QStringLiteral("Hết hạn quyền Quản trị viên tạm thời"),
            msg
        );

        teardownShell();
        delete currentManager;
        currentManager = nullptr;
        currentManagerId.clear();
        m_startedAsTempAdmin = false;
        m_forceLogoutInProgress = false;
        showLogin();
    });
}
