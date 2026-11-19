#include "Human.h"

Human::Human(std::string id, std::string fullName, std::string phone, bool gender, int age)
    : id(id), fullName(fullName), phone(phone), gender(gender), age(age) {}

std::string Human::getID() const { return id; }
std::string Human::getfullName() const { return fullName; }
std::string Human::getPhone() const { return phone; }
bool Human::getGender() const { return gender; }
int Human::getAge() const { return age; }

void Human::setID(const std::string &identifier) { id = identifier; }
void Human::setfullName(const std::string &fn) { fullName = fn; }
void Human::setPhone(const std::string &p) { phone = p; }
void Human::setGender(bool g) { gender = g; }
void Human::setAge(int a) { age = a; }
