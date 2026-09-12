#ifndef HUMAN_H
#define HUMAN_H

#include <iostream>
#include <string>

using namespace std;

class Human {
protected: // Dùng protected để các lớp con (Doctor, Patient) có thể truy cập được
    string id;
    string name;
    int age;
    string gender;

public:
    // Constructor mặc định
    Human(string _id = "", string _name = "", int _age = 0, string _gender = "")
        : id(_id), name(_name), age(_age), gender(_gender) {}

    // Hàm ảo (virtual) để các lớp con ghi đè (override)
    virtual void displayInfo() const {
        cout << "ID: " << id << " | Ten: " << name 
             << " | Tuoi: " << age << " | Gioi tinh: " << gender << endl;
    }

    // Cần có virtual destructor cho lớp cha
    virtual ~Human() = default;
};

#endif // HUMAN_H