#ifndef APPOINTMENT_H
#define APPOINTMENT_H

#include "Date.h"
#include "../Data_structures/LinkList.h"
#include <string>

class Appointment {
private:
    std::string id;
    std::string patientId;
    std::string doctorId;
    Date dateSet;
    Date dateGet;
    int state;

public:
    Appointment(std::string id = "", std::string patientId = "",
                std::string doctorId = "", const Date &dateSet = Date(),
                const Date &dateGet = Date(), int state = 0);

    std::string getId() const;
    std::string getPatientId() const;
    std::string getDoctorId() const;
    Date getDateSet() const;
    Date getDateGet() const;
    int getState() const;

    void setId(const std::string &id);
    void setPatientId(const std::string &patientId);
    void setDoctorId(const std::string &doctorId);
    void setDateSet(const Date &dateSet);
    void setDateGet(const Date &dateGet);
    void setState(int state);

    void displayInfo() const;

    // Ngày có định dạng yyyy-MM-dd HH:mm trong Appointment.txt.
    static LinkList<Appointment> loadFromFile(const std::string &filename);
};

#endif // APPOINTMENT_H
