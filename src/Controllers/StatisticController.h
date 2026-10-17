#ifndef STATISTICCONTROLLER_H
#define STATISTICCONTROLLER_H

struct ClinicStatistics {
    int totalDoctors;
    int totalPatients;
    int totalAppointments;
    int completedAppointments;
    int pendingAppointments;
    int canceledAppointments;
};

class StatisticController {
public:
    StatisticController();

    // Thu thập dữ liệu thống kê tổng hợp từ các Model
    ClinicStatistics getClinicStatistics();
};

#endif // STATISTICCONTROLLER_H
