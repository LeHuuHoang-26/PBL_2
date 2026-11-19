#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QString>
#include <QDate>
#include <string>
#include <QCloseEvent>

#include "LinkList.h"
#include "Manager.h"
#include "Patient.h"
#include "Doctor.h"
#include "Appointment.h"
#include "Symptom.h"
#include "SymptomPatient.h"
#include "SymptomPatientDetail.h"
#include "PermissionManager.h"

class QStackedWidget;
class QPushButton;
class QLabel;
class QTimer;

class AppointmentView;
class PatientView;
class DoctorView;
class AccountView;
class StatisticView;
class Login;

enum class SaveScope {
    All,
    Patient,
    Doctor,
    Appointment,
    Account
};

enum class SaveAction {
    Normal,
    CloseOrLogout
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    bool initApp();
    bool isInitialized() const { return appInitialized; }
    void showLogin();
    void handleLogin();
    void handleLogout();
    void setPage(int index);
    void refreshPage(int index);
    bool saveAll(SaveScope scope = SaveScope::All, SaveAction action = SaveAction::Normal);

    LinkList<Manager> *getManagers() const { return managers; }
    LinkList<Patient> *getPatients() const { return patients; }
    LinkList<Doctor> *getDoctors() const { return doctors; }
    LinkList<Symptom> *getSymptoms() const { return symptoms; }
    LinkList<SymptomPatient> *getSymptomPatients() const { return symptomPatients; }
    LinkList<SymptomPatientDetail> *getSymptomPatientDetails() const { return symptomPatientDetails; }
    LinkList<Appointment> *getAppointments() const { return appointments; }
    LinkList<PermissionManager> *getPermissionRecords() const { return permissionRecords; }
    Manager *getCurrentManager() const;
    void resetTabOrderToId(int tabIndex);
    void updateUserProfileDisplay();

    bool isRootAdmin() const;
    bool isAdministrator() const;
    bool hasPermission(const std::string &permissionId) const;

    QString specialty(const std::string &id) const;
    Patient *findPatient(const std::string &id) const;
    Doctor *findDoctor(const std::string &id) const;
    SymptomPatient *findSymptomPatient(const std::string &id) const;
    Patient *findPatientForAppointment(const Appointment &a) const;
    std::string getPatientIdForAppointment(const Appointment &a) const;
    QString getSymptomNamesForAppointment(const Appointment &a) const;
    Date getDateSetForAppointment(const Appointment &a) const;
    bool isHoliday(const QDate &date) const;
    QString holidayNote(const QDate &date) const;
    std::string nextAppointmentId() const;

    bool showBookingDialog(const QDate &defaultDate = QDate::currentDate(), const std::string &preselectedPatientId = "");
    void editAppointmentDialog(const std::string &id);
    void showRescheduledSummaryDialog(QWidget *parent, const QList<struct RescheduledResultItem> &items, int totalAffected, const QString &customTitle = "");

    QString style() const;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    LinkList<Manager> *managers = nullptr;
    LinkList<Patient> *patients = nullptr;
    LinkList<Doctor> *doctors = nullptr;
    LinkList<Symptom> *symptoms = nullptr;
    LinkList<SymptomPatient> *symptomPatients = nullptr;
    LinkList<SymptomPatientDetail> *symptomPatientDetails = nullptr;
    LinkList<Appointment> *appointments = nullptr;
    LinkList<PermissionManager> *permissionRecords = nullptr;

    Manager *currentManager = nullptr;
    std::string currentManagerId;
    QWidget *root = nullptr;
    QStackedWidget *pages = nullptr;
    QWidget *sidebar = nullptr;
    QList<QPushButton *> navButtons;
    int activeNav = 0;

    QStackedWidget *windowStack = nullptr;
    Login *loginView = nullptr;
    AppointmentView *appointmentView = nullptr;
    PatientView *patientView = nullptr;
    DoctorView *doctorView = nullptr;
    AccountView *accountView = nullptr;
    StatisticView *statisticView = nullptr;

    QLabel *sidebarAvatarLabel = nullptr;
    QLabel *sidebarNameLabel = nullptr;
    QLabel *sidebarRoleLabel = nullptr;
    QTimer *clockTimer = nullptr;
    QObject *cursorLeaveFilter = nullptr;
    bool appInitialized = false;
    bool m_startedAsTempAdmin = false;
    bool m_forceLogoutInProgress = false;

    void updateNavigation();
    void setupShell();
    void handleTemporaryAdminExpired();
};

#endif
