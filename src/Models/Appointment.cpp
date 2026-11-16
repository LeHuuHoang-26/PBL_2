#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "Appointment.h"
#include "libPBL2.h"
#include "../Utils/AppExceptions.h"

using namespace std;

Appointment::Appointment(string id, string symptomPatientId, string doctorId, const Date &dateGet, int state)
    : id(id), symptomPatientId(symptomPatientId), doctorId(doctorId), dateGet(dateGet), state(state) {}

string Appointment::getId() const { return id; }
string Appointment::getSymptomPatientId() const { return symptomPatientId; }
string Appointment::getDoctorId() const { return doctorId; }
Date Appointment::getDateGet() const { return dateGet; }
int Appointment::getState() const { return state; }

void Appointment::setId(const string &value) { id = value; }
void Appointment::setSymptomPatientId(const string &value) { symptomPatientId = value; }
void Appointment::setDoctorId(const string &value) { doctorId = value; }
void Appointment::setDateGet(const Date &value) { dateGet = value; }
void Appointment::setState(int value) { state = value; }

int Appointment::computeEffectiveState(int curDay, int curMonth, int curYear, int curHour, int curMin) const {
    if (state == -2) return -2; // Dời lịch
    if (state == 1) return 1;   // Đã khám
    if (state == -1) return -1; // Trễ hẹn

    Date ad = getDateGet();
    if (ad.getYear() < curYear ||
       (ad.getYear() == curYear && ad.getMonth() < curMonth) ||
       (ad.getYear() == curYear && ad.getMonth() == curMonth && ad.getDay() < curDay)) {
        return -1; // Trễ hẹn (ngày hẹn đã qua)
    }
    if (ad.getYear() == curYear && ad.getMonth() == curMonth && ad.getDay() == curDay) {
        int apptMinutes = ad.getHour() * 60 + ad.getMinute();
        int curMinutes = curHour * 60 + curMin;
        if (apptMinutes + 30 <= curMinutes) {
            return -1; // Trễ hẹn (hôm nay nhưng đã quá 30 phút so với giờ hẹn)
        }
    }
    return 0; // Chưa khám (chưa đến giờ)
}

bool Appointment::isDefault() const {
    return id.empty() && symptomPatientId.empty() && doctorId.empty();
}

void Appointment::setDefault() {
    id = "";
    symptomPatientId = "";
    doctorId = "";
    dateGet = Date();
    state = 0;
}

string Appointment::toString() const {
    string stStr = to_string(state);
    if (stStr.length() < 7) {
        stStr = string(7 - stStr.length(), ' ') + stStr;
    }
    return libPBL2::padRightUtf8(id, 5) + " | "
         + libPBL2::padRightUtf8(symptomPatientId, 6) + " | "
         + libPBL2::padRightUtf8(doctorId, 5) + " | "
         + libPBL2::padRightUtf8(dateGet.toString(), 16) + " | "
         + stStr;
}

Appointment Appointment::loadFromString(const string &str) {
    if (str.empty()) return Appointment();
    stringstream ss(str);
    string token;
    vector<string> parts;
    while (getline(ss, token, '|')) {
        libPBL2::trim(token);
        parts.push_back(token);
    }

    if (parts.size() < 5) {
        return Appointment();
    }

    string id = parts[0];
    string spId = parts[1];
    string doctorId = parts[2];
    string dateGetStr;
    string stateStr;

    if (parts.size() >= 6) {
        // Cấu trúc cũ (tương thích ngược): ID | SP_ID | D_ID | DateSet | DateGet | State
        dateGetStr = parts[4];
        stateStr = parts[5];
    } else {
        // Cấu trúc mới: ID | SP_ID | D_ID | DateGet | State
        dateGetStr = parts[3];
        stateStr = parts[4];
    }

    if (id.empty() || spId.empty() || doctorId.empty())
        return Appointment();

    Date dGet = Date::loadFromString(dateGetStr);
    int st = libPBL2::safeParseInt(stateStr, "Trạng thái cuộc hẹn", 0);

    return Appointment(id, spId, doctorId, dGet, st);
}

LinkList<Appointment>* Appointment::loadFromFile(const string &path) {
    ifstream f(path);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để đọc dữ liệu.");
    }

    LinkList<Appointment> *list = nullptr;
    try {
        list = new LinkList<Appointment>;
        string tmp;
        getline(f, tmp); // Bỏ qua tiêu đề
        getline(f, tmp); // Bỏ qua dòng phân cách

        string line;
        int lineNo = 2;
        while (getline(f, line)) {
            lineNo++;
            libPBL2::trim(line);
            if (line.empty()) continue;
            try {
                Appointment app = Appointment::loadFromString(line);
                if (!app.isDefault()) {
                    list->insert(app);
                }
            } catch (const AppException &e) {
                throw DataFormatException(path, lineNo, e.what());
            } catch (const std::exception &e) {
                throw DataFormatException(path, lineNo, e.what());
            }
        }

        f.close();
        return list;
    } catch (...) {
        delete list;
        f.close();
        throw;
    }
}

bool Appointment::saveToFile(const string &path, const LinkList<Appointment> *list) {
    if (!list || list->getHeader() == nullptr)
        return false;

    string tempPath = path + ".tmp";
    ofstream f(tempPath);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp tạm để ghi dữ liệu.");
    }

    try {
        f << "A_ID  | SP_ID  | D_ID  | A_DateGet        | A_State\n";
        f << "------+--------+-------+------------------+---------\n";

        LinkList<Appointment>::Node *tmp = list->getHeader()->next;
        while (tmp != nullptr && tmp->next != nullptr) {
            if (!tmp->info.isDefault()) {
                f << tmp->info.toString() << "\n";
            }
            tmp = tmp->next;
        }

        f.close();
        libPBL2::safeAtomicCommit(tempPath, path);
        return true;
    } catch (...) {
        f.close();
        std::remove(tempPath.c_str());
        throw;
    }
}
