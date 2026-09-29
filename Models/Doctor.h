#ifndef DOCTOR_H
#define DOCTOR_H

#include "human.h"
#include <string>


class Doctor : public Human {
private:
    string phoneNumbers; 
    string spId; // Mã chuyên ngành

public:
    // Hàm khởi tạo (Các giá trị mặc định = "" chỉ được phép đặt ở file .h)
    Doctor(string _id = "", string _lastName = "", string _middleName = "", 
           string _firstName = "", string _phoneNumbers = "", string _gender = "", 
           string _spId = "");

    // Hàm hủy
    virtual ~Doctor();

    // Ghi đè hàm hiển thị từ lớp Human
    void displayInfo() const override;

    // Các hàm Getter
    string getPhoneNumbers() const;
    string getSpId() const;

    // Các hàm Setter
    void setPhoneNumbers(string _phone);
    void setSpId(string _spId);
};

#endif // DOCTOR_H