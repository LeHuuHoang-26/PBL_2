#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <string>

class DoctorView;
class PatientView;
class AppointmentView;
class StatisticView;
class AccountView;

class MainWindow : public QMainWindow {
    Q_OBJECT
private:
    QStackedWidget *stackedWidget;
    QPushButton *btnDoctor;
    QPushButton *btnPatient;
    QPushButton *btnAppointment;
    QPushButton *btnStatistic;
    QPushButton *btnAccount;
    QLabel *userStatusLabel;

    DoctorView *doctorView;
    PatientView *patientView;
    AppointmentView *appointmentView;
    StatisticView *statisticView;
    AccountView *accountView;

public:
    explicit MainWindow(const std::string &userName = "Admin", int role = 3, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void switchPage(int index);
};

#endif // MAINWINDOW_H
