#ifndef PATIENT_H
#define PATIENT_H

#include "human.h"
#include "Symptom.h" // Nhúng thêm model Symptom theo cấu trúc thư mục của bạn
#include <vector>

class Patient : public Human {
private:
    string bloodType;              // Nhóm máu
    string medicalHistory;         // Tiền sử bệnh
    // vector<Symptom> symptoms;   // Có thể thêm mảng các triệu chứng nếu cần

public:
    Patient(string _id = "", string _name = "", int _age = 0, string _gender = "", 
            string _bloodType = "", string _history = "")
        : Human(_id, _name, _age, _gender), bloodType(_bloodType), medicalHistory(_history) {}

    // Ghi đè hàm in thông tin
    void displayInfo() const override {
        cout << "--- THONG TIN BENH NHAN ---" << endl;
        Human::displayInfo(); 
        cout << "Nhom mau: " << bloodType 
             << " | Tien su benh: " << medicalHistory << endl;
    }
};

#endif // PATIENT_H