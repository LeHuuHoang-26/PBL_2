#ifndef APPOINTMENTCONTROLLER_H
#define APPOINTMENTCONTROLLER_H

#include "Appointment.h"
#include "LinkList.h"
#include <string>
#include <vector>

class AppointmentController {
public:
    AppointmentController();

    // Lấy toàn bộ danh sách lịch hẹn
    LinkList<Appointment>* getAllAppointments();

    // Tìm kiếm lịch hẹn theo từ khóa (mã hẹn, mã BN, mã BS)
    std::vector<Appointment> searchAppointments(const std::string &keyword);

    // Lọc lịch hẹn theo trạng thái (0: chờ khám, 1: đã khám, 2: đã hủy)
    std::vector<Appointment> filterByState(int state);

    // Lấy tổng số lượng lịch hẹn
    int getTotalAppointmentCount();
};

#endif // APPOINTMENTCONTROLLER_H
