#include "../Models/human.h"
#include <string>
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