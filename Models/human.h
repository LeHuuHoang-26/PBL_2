#ifndef HUMAN_H
#define HUMAN_H

#include <string>

using namespace std;

class Human {
protected: 
    string id;
    string fullName;
    int phone;
    bool gender;
    int age; 

public:
    Human(string id = "", string fullName = "", int phone = 0, bool gender = 0, int age = 0);
    virtual ~Human() = default;
};

#endif 