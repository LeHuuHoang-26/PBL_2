#include "Doctor.h"
#include "libPBL2.h"
#include "../Utils/AppExceptions.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace std;

Doctor::Doctor(string _id, string _fullName, string _phone, bool _gender, int _age, string _spId, bool _state, string _roomId, double _fee, double _rating)
    : Human(_id, _fullName, _phone, _gender, _age), spId(_spId), state(_state), roomId(_roomId), fee(_fee), rating(_rating) {}

bool Doctor::isDefault() const {
    return id.empty() && fullName.empty();
}

void Doctor::setDefault() {
    id = "";
    fullName = "";
    phone = "";
    gender = true;
    age = 0;
    spId = "";
    state = true;
    roomId = "";
    fee = 0.0;
    rating = 5.0;
}

string Doctor::getSpId() const { return spId; }
bool Doctor::getState() const { return state; }
string Doctor::getRoomId() const { return roomId; }
double Doctor::getFee() const { return fee; }
double Doctor::getRating() const { return rating; }

void Doctor::setSpId(string _spId) { this->spId = _spId; }
void Doctor::setState(bool _state) { this->state = _state; }
void Doctor::setRoomId(string _roomId) { this->roomId = _roomId; }
void Doctor::setFee(double _fee) { this->fee = _fee; }
void Doctor::setRating(double _rating) { this->rating = _rating; }

string Doctor::toString() const {
    ostringstream oss;
    oss << libPBL2::padRightUtf8(id, 6) << " | "
        << libPBL2::padRightUtf8(fullName, 20) << " | "
        << libPBL2::padRightUtf8(phone, 10) << " | "
        << libPBL2::padRightUtf8(gender ? "1" : "0", 8) << " | "
        << libPBL2::padRightUtf8(to_string(age), 5) << " | "
        << libPBL2::padRightUtf8(spId, 6) << " | "
        << libPBL2::padRightUtf8(state ? "1" : "0", 7) << " | "
        << libPBL2::padRightUtf8(roomId, 8) << " | "
        << libPBL2::padRightUtf8([&]() {
            ostringstream fss;
            fss << fixed << setprecision(1) << fee;
            return fss.str();
        }(), 16) << " | "
        << libPBL2::padRightUtf8([&]() {
            ostringstream rss;
            rss << fixed << setprecision(1) << rating;
            return rss.str();
        }(), 6);
    return oss.str();
}

Doctor Doctor::loadFromString(const string &str) {
    if (str.empty()) return Doctor();
    stringstream ss(str);
    string id, fullName, phone, genderStr, ageStr;
    string spId, stateStr, roomId, feeStr, ratingStr;

    if (!getline(ss, id, '|') ||
        !getline(ss, fullName, '|') ||
        !getline(ss, phone, '|') ||
        !getline(ss, genderStr, '|') ||
        !getline(ss, ageStr, '|') ||
        !getline(ss, spId, '|') ||
        !getline(ss, stateStr, '|') ||
        !getline(ss, roomId, '|') ||
        !getline(ss, feeStr, '|') ||
        !getline(ss, ratingStr)) {
        return Doctor();
    }

    libPBL2::trim(id);
    libPBL2::trim(fullName);
    libPBL2::trim(phone);
    libPBL2::trim(genderStr);
    libPBL2::trim(ageStr);
    libPBL2::trim(spId);
    libPBL2::trim(stateStr);
    libPBL2::trim(roomId);
    libPBL2::trim(feeStr);
    libPBL2::trim(ratingStr);

    if (id.empty()) return Doctor();

    bool gender = (genderStr == "1");
    int age = 0;
    try {
        if (!ageStr.empty()) age = stoi(ageStr);
    } catch (...) {}

    bool state = (stateStr == "1");

    double fee = 0.0;
    try {
        if (!feeStr.empty()) fee = stod(feeStr);
    } catch (...) {}

    double rating = 5.0;
    try {
        size_t pipePos = ratingStr.find('|');
        if (pipePos != string::npos) ratingStr = ratingStr.substr(0, pipePos);
        libPBL2::trim(ratingStr);
        if (!ratingStr.empty()) rating = stod(ratingStr);
    } catch (...) {}

    return Doctor(id, fullName, phone, gender, age, spId, state, roomId, fee, rating);
}

LinkList<Doctor>* Doctor::loadFromFile(const string &path) {
    ifstream f(path);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để đọc dữ liệu.");
    }

    LinkList<Doctor> *doctorList = nullptr;
    try {
        doctorList = new LinkList<Doctor>;
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
                Doctor doc = Doctor::loadFromString(line);
                if (!doc.isDefault()) {
                    doctorList->insert(doc);
                }
            } catch (const AppException &) {
                throw;
            } catch (const std::exception &e) {
                throw DataFormatException(path, lineNo, e.what());
            }
        }

        f.close();
        return doctorList;
    } catch (...) {
        delete doctorList;
        f.close();
        throw;
    }
}

bool Doctor::saveToFile(const string &path, const LinkList<Doctor> *list) {
    if (!list || list->getHeader() == nullptr)
        return false;

    ofstream f(path);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để ghi dữ liệu.");
    }

    f << "D_ID   | D_FullName           | D_Phone    | D_Gender | D_Age | Sp_ID  | D_State | Room_ID  | Consultation_Fee | Rating\n";
    f << "-------+----------------------+------------+----------+-------+--------+---------+----------+------------------+-------\n";

    LinkList<Doctor>::Node *tmp = list->getHeader()->next;
    while (tmp != nullptr && tmp->next != nullptr) {
        if (!tmp->info.isDefault()) {
            f << tmp->info.toString() << "\n";
        }
        tmp = tmp->next;
    }

    f.close();
    return true;
}
