#ifndef PATIENTCONTROLLER_H
#define PATIENTCONTROLLER_H

#include "Patient.h"
#include "QueuePatient.h"
#include "LinkList.h"
#include "Queue.h"
#include <string>
#include <vector>

class PatientController {
public:
    PatientController();

    // Lấy toàn bộ danh sách bệnh nhân
    LinkList<Patient>* getAllPatients();

    // Tìm kiếm bệnh nhân theo từ khóa (tên, mã, CCCD, SĐT)
    std::vector<Patient> searchPatients(const std::string &keyword);

    // Lấy hàng đợi bệnh nhân
    Queue<QueuePatient>* getPatientQueue();

    // Lấy tổng số lượng bệnh nhân
    int getTotalPatientCount();
};

#endif // PATIENTCONTROLLER_H
