#ifndef PATIENT_H
#define PATIENT_H

#include "human.h"
#include <string>
#include <iostream>

using namespace std;

class Patient : public Human {
private:
    string phoneNumbers; 
    string cccd;         
    int age;            

public:
    // Constructor 
    Patient(string _id = "", string _lastName = "", string _middleName = "", 
            string _firstName = "", string _phoneNumbers = "", string _gender = "", 
            string _cccd = "", int _age = 0)
        : Human(_id, _lastName, _middleName, _firstName, _gender),
          phoneNumbers(_phoneNumbers), cccd(_cccd), age(_age) {}

    void displayInfo() const override {
        cout << "==== THONG TIN BENH NHAN ====" << endl;
        Human::displayInfo(); 
        
        cout << "Tuoi: " << age << endl;
        cout << "So CCCD: " << cccd << endl;
        cout << "So dien thoai: " << phoneNumbers << endl;
        cout << "=============================" << endl;
    }

    string getPhoneNumbers() const { return phoneNumbers; }
    string getCccd() const { return cccd; }
    int getAge() const { return age; }

    void setPhoneNumbers(string _phone) { phoneNumbers = _phone; }
    void setCccd(string _cccd) { cccd = _cccd; }
    void setAge(int _age) { age = _age; }
};

#endif 