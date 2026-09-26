#include "Human.h"
#include <string>
using namespace std;

Human::Human(string id, string fullName, int phone, bool gender, int age)
    : id(id), fullName(fullName), phone(phone), gender(gender), age(age) {}