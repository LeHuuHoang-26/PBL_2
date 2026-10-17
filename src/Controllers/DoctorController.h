#ifndef DOCTORCONTROLLER_H
#define DOCTORCONTROLLER_H

#include "Doctor.h"
#include "LinkList.h"
#include <string>
#include <vector>

class DoctorController {
public:
    DoctorController();

    // Lấy toàn bộ danh sách bác sĩ
    LinkList<Doctor>* getAllDoctors();

    // Tìm kiếm bác sĩ theo từ khóa (tên hoặc mã)
    std::vector<Doctor> searchDoctors(const std::string &keyword);

    // Lấy thông tin bác sĩ theo ID
    bool getDoctorById(const std::string &id, Doctor &outDoctor);

    // Lấy tổng số lượng bác sĩ
    int getTotalDoctorCount();
};

#endif // DOCTORCONTROLLER_H
