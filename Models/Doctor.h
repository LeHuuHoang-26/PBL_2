#ifndef DOCTOR_H
#define DOCTOR_H

#include "human.h"

class Doctor : public Human {
private:
    string specialty;       // Chuyên khoa
    int yearsOfExperience;  // Số năm kinh nghiệm

public:
    Doctor(string _id = "", string _name = "", int _age = 0, string _gender = "", 
           string _specialty = "", int _experience = 0)
        : Human(_id, _name, _age, _gender), specialty(_specialty), yearsOfExperience(_experience) {}

    // Ghi đè hàm in thông tin
    void displayInfo() const override {
        cout << "--- THONG TIN BAC SI ---" << endl;
        Human::displayInfo(); // Gọi lại hàm in của lớp cha
        cout << "Chuyen khoa: " << specialty 
             << " | Kinh nghiem: " << yearsOfExperience << " nam" << endl;
    }
};

#endif // DOCTOR_H