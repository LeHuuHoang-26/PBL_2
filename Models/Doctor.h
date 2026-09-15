#ifndef DOCTOR_H
#define DOCTOR_H

#include "human.h"
#include <string>
#include <iostream>

using namespace std;

class Doctor : public Human {
private:
    string phoneNumbers; 
    string spId; //chuyen nganh

public:
    Doctor(string _id = "", string _lastName = "", string _middleName = "", 
           string _firstName = "", string _phoneNumbers = "", string _gender = "", 
           string _spId = "")
        : Human(_id, _lastName, _middleName, _firstName, _gender),
          phoneNumbers(_phoneNumbers), spId(_spId) {}

    void displayInfo() const override {
        cout << "==== THONG TIN BAC SI ====" << endl;
        Human::displayInfo(); 
        cout << "So dien thoai: " << phoneNumbers << endl;
        cout << "Ma chuyen khoa (Sp_ID): " << spId << endl;
        cout << "==========================" << endl;
    }

    string getPhoneNumbers() const { return phoneNumbers; }
    string getSpId() const { return spId; }

    void setPhoneNumbers(string _phone) { phoneNumbers = _phone; }
    void setSpId(string _spId) { spId = _spId; }
};

#endif 