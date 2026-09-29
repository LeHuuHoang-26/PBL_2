#include "human.h"
#include <string>
#include <iostream>
using namespace std;

Human::Human(string id, string fullName, string phone, bool gender, int age)
    : id(id), fullName(fullName), phone(phone), gender(gender), age(age) {}

bool operator ==(const Human &H1, const Human &H2) {
    return H1.fullName  == H2.fullName
        && H1.phone     == H2.phone
        && H1.gender    == H2.gender
        && H1.age       == H2.age;
}

void Human::displayInfo() const {
    cout << "ID: " << id << "\n"
         << "FullName: " << fullName << "\n"
         << "Phone: " << phone << "\n"
         << "Gender: " << (gender ? "Nam" : "Nu") << "\n"
         << "Age: " << age << "\n";
}

string Human::getID() const {return this->id;}
string Human::getfullName() const {return this->fullName;}
string Human::getPhone() const {return this->phone;}
bool Human::getGender() const {return this->gender;}
int Human::getAge() const {return this->age;}

string Human::setID(string id) {this->id = id;}
string Human::setfullName(string fullName) {this->fullName = fullName;}
string Human::setPhone(string phone) {this->phone = phone;}
bool Human::setGender(bool gender) {this->gender = gender;}
int Human::setAge(int age) {this->age = age;}