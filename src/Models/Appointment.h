#ifndef APPOINTMENT_H
#define APPOINTMENT_H

#include "Date.h"
#include "LinkList.h"
#include <string>

class Appointment {
private:
    std::string id;
    std::string symptomPatientId;
    std::string doctorId;
    Date dateGet;
    int state;
    double paidFee;

public:
    Appointment(std::string id = "", std::string symptomPatientId = "", std::string doctorId = "", const Date &dateGet = Date(), int state = 0, double paidFee = -1.0);
    Appointment(const Appointment &) = default;
    Appointment &operator=(const Appointment &) = default;
    Appointment(Appointment &&) noexcept = default;
    Appointment &operator=(Appointment &&) noexcept = default;
    ~Appointment() = default;

    std::string getId() const;
    std::string getSymptomPatientId() const;
    std::string getDoctorId() const;
    Date getDateGet() const;
    int getState() const;
    double getPaidFee() const;
    bool hasPaidFee() const;

    void setId(const std::string &id);
    void setSymptomPatientId(const std::string &spId);
    void setDoctorId(const std::string &doctorId);
    void setDateGet(const Date &dateGet);
    void setState(int state);
    void setPaidFee(double fee);

    int computeEffectiveState(int currentDay, int currentMonth, int currentYear, int currentHour, int currentMinute) const;

    bool isDefault() const;
    void setDefault();

    std::string toString() const;
    static Appointment loadFromString(const std::string &str);
    static LinkList<Appointment>* loadFromFile(const std::string &path);
    static bool saveToFile(const std::string &path, const LinkList<Appointment> *list);
};

#endif
