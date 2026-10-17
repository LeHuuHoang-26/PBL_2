#include "../Models/Appointment.h"
#include "../Models/libPBL2.h"

#include <fstream>
#include <exception>
#include <iostream>
#include <sstream>

namespace {
bool parseDate(const std::string &value, Date &date) {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    char firstSeparator = '\0';
    char secondSeparator = '\0';
    char timeSeparator = '\0';
    std::istringstream input(value);

    if (!(input >> year >> firstSeparator >> month >> secondSeparator >> day
                >> hour >> timeSeparator >> minute)
        || firstSeparator != '-' || secondSeparator != '-' || timeSeparator != ':'
        || hour < 0 || hour >= 24 || minute < 0 || minute >= 60)
        return false;

    input >> std::ws;
    if (!input.eof() || year < 1 || month < 1 || month > 12)
        return false;

    date.insert(day, month, year, hour, minute);
    return day >= 1 && day <= date.posofdayinmonth();
}
}

Appointment::Appointment(std::string id, std::string patientId,
                         std::string doctorId, const Date &dateSet,
                         const Date &dateGet, int state)
    : id(id), patientId(patientId), doctorId(doctorId),
      dateSet(dateSet), dateGet(dateGet), state(state) {}

std::string Appointment::getId() const { return id; }
std::string Appointment::getPatientId() const { return patientId; }
std::string Appointment::getDoctorId() const { return doctorId; }
Date Appointment::getDateSet() const { return dateSet; }
Date Appointment::getDateGet() const { return dateGet; }
int Appointment::getState() const { return state; }

void Appointment::setId(const std::string &value) { id = value; }
void Appointment::setPatientId(const std::string &value) { patientId = value; }
void Appointment::setDoctorId(const std::string &value) { doctorId = value; }
void Appointment::setDateSet(const Date &value) { dateSet = value; }
void Appointment::setDateGet(const Date &value) { dateGet = value; }
void Appointment::setState(int value) { state = value; }

void Appointment::displayInfo() const {
    std::cout << "A_ID: " << id << '\n'
              << "P_ID: " << patientId << '\n'
              << "D_ID: " << doctorId << '\n'
              << "A_DateSet: ";
    dateSet.show();
    std::cout << "A_DateGet: ";
    dateGet.show();
    std::cout << "A_State: " << state << '\n';
}

LinkList<Appointment> Appointment::loadFromFile(const std::string &filename) {
    LinkList<Appointment> appointments;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Khong the mo file: " << filename << '\n';
        return appointments;
    }

    std::string line;
    std::getline(file, line); // Bỏ qua tiêu đề.
    std::getline(file, line); // Bỏ qua dòng phân cách.

    while (std::getline(file, line)) {
        libPBL2::trim(line);
        if (line.empty())
            continue;

        std::istringstream row(line);
        std::string id, patientId, doctorId, dateSetText, dateGetText, stateText;
        if (!std::getline(row, id, '|')
            || !std::getline(row, patientId, '|')
            || !std::getline(row, doctorId, '|')
            || !std::getline(row, dateSetText, '|')
            || !std::getline(row, dateGetText, '|')
            || !std::getline(row, stateText, '|'))
            continue;

        libPBL2::trim(id);
        libPBL2::trim(patientId);
        libPBL2::trim(doctorId);
        libPBL2::trim(dateSetText);
        libPBL2::trim(dateGetText);
        libPBL2::trim(stateText);

        Date dateSet, dateGet;
        if (id.empty() || patientId.empty() || doctorId.empty()
            || !parseDate(dateSetText, dateSet)
            || !parseDate(dateGetText, dateGet))
            continue;

        try {
            appointments.insert(Appointment(id, patientId, doctorId,
                                            dateSet, dateGet, std::stoi(stateText)));
        } catch (const std::exception &) {
            // Bỏ qua dòng có trạng thái không phải số nguyên.
        }
    }

    return appointments;
}
