#ifndef HUMAN_H
#define HUMAN_H

#include <string>

using namespace std;

class Human {
protected: 
    string id;
    string fullName;
    string phone;
    bool gender;
    int age; 

public:
    Human(string id = "", string fullName = "", string phone = 0, bool gender = 0, int age = 0);
    virtual ~Human() = default;
    friend bool operator ==(const Human &, const Human &);
};

#endif 