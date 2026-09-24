#ifndef HUMAN_H
#define HUMAN_H

#include <iostream>
#include <string>

using namespace std;

class Human {
protected: 
    string id;
    string lastName;
    string middleName;
    string firstName;
    string gender; 

public:
    Human(string _id = "", string _lastName = "", string _middleName = "", 
          string _firstName = "", string _gender = "")
        : id(_id), lastName(_lastName), middleName(_middleName), 
          firstName(_firstName), gender(_gender) {}

    virtual ~Human() = default;
};

#endif 