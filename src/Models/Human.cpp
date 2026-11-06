#include "Human.h"
#include <string>
#include <iostream>
using namespace std;

Human::Human(string id, string fullName, string phone, bool gender, int age)
    : id(id), fullName(fullName), phone(phone), gender(gender), age(age) {}

bool Human::operator==(const Human &other) const {
    if (this == &other)
        return true;
    return this->fullName == other.fullName
        && this->phone == other.phone
        && this->gender == other.gender
        && this->age == other.age;
}

string Human::getID() const {return this->id;}
string Human::getfullName() const {return this->fullName;}
string Human::getPhone() const {return this->phone;}
bool Human::getGender() const {return this->gender;}
int Human::getAge() const {return this->age;}

void Human::setID(string id) {this->id = id;}
void Human::setfullName(string fullName) {this->fullName = fullName;}
void Human::setPhone(string phone) {this->phone = phone;}
void Human::setGender(bool gender) {this->gender = gender;}
void Human::setAge(int age) {this->age = age;}
