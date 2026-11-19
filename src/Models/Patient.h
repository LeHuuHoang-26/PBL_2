#ifndef PATIENT_H
#define PATIENT_H

#include "Human.h"
#include "LinkList.h"
#include <string>

class Patient : public Human {
private:
    std::string CCCD;
    int state;

public:
    Patient();
    Patient(std::string id, std::string fullName, std::string phone, bool gender, int age, std::string CCCD, int state = 1);
    Patient(const Patient &P);
    Patient &operator=(const Patient &P) = default;
    Patient(Patient &&P) noexcept = default;
    Patient &operator=(Patient &&P) noexcept = default;
    ~Patient() = default;

    void setDefault();
    bool isDefault() const;

    std::string getCCCD() const;
    int getState() const;

    void setCCCD(const std::string &CCCD);
    void setState(int state);

    std::string toString() const;
    static Patient loadFromString(const std::string &str);
    static LinkList<Patient>* loadFromFile(const std::string &path);
    static bool saveToFile(const std::string &path, const LinkList<Patient> *list);
    static std::string generateNextId(const LinkList<Patient> *list);
};

#endif
