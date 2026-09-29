#include "../Models/Doctor.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

Doctor::Doctor(string _id, string _fullName, string _phone, 
               bool _gender, int _age, string _spId, bool _state,
               string _roomId, double _fee, double _rating)
    : spId(_spId), state(_state), roomId(_roomId), fee(_fee), rating(_rating) 
{
    this->id = _id;
    this->fullName = _fullName;
    this->phone = _phone;
    this->gender = _gender;
    this->age = _age;
}

Doctor::~Doctor() {
}

void Doctor::displayInfo() const {
    cout << "--- DOCTOR (D_ID | D_FullName | D_Phone | D_Gender | D_Age | Sp_ID | D_State | D_RoomID | D_Fee | D_Rating) ---\n"
         << "D_ID: " << id << "\n"
         << "D_FullName: " << fullName << "\n"
         << "D_Phone: " << phone << "\n"
         << "D_Gender: " << (gender ? "Nam" : "Nu") << "\n"
         << "D_Age: " << age << "\n"
         << "Sp_ID: " << spId << "\n"
         << "D_State: " << (state ? "Dang lam viec" : "Nghi") << "\n"
         << "D_RoomID: " << roomId << "\n"
         << "D_Fee: " << fee << "\n"
         << "D_Rating: " << rating << "\n";
}

// Getter
string Doctor::getSpId() const { return spId; }
bool Doctor::getState() const { return state; }
string Doctor::getRoomId() const { return roomId; }
double Doctor::getFee() const { return fee; }
double Doctor::getRating() const { return rating; }

// Setter
void Doctor::setSpId(string _spId) { this->spId = _spId; }
void Doctor::setState(bool _state) { this->state = _state; }
void Doctor::setRoomId(string _roomId) { this->roomId = _roomId; }
void Doctor::setFee(double _fee) { this->fee = _fee; }
void Doctor::setRating(double _rating) { this->rating = _rating; }

// -------------------------------------------------------
// Hàm nội bộ: trim khoảng trắng đầu/cuối chuỗi
// -------------------------------------------------------
static string trimStr(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}

// -------------------------------------------------------
// Đọc danh sách bác sĩ từ file Doctor.txt
// Format: D_ID | D_FullName | D_Phone | D_Gender | D_Age |
//         Sp_ID | D_State | Room_ID | Consultation_Fee | Rating
// Dòng 1: tiêu đề, Dòng 2: phân cách ---+---, từ Dòng 3: dữ liệu
// -------------------------------------------------------
LinkList<Doctor> Doctor::loadFromFile(const string& filename) {
    LinkList<Doctor> listDoctors;
    ifstream file(filename);

    if (!file.is_open()) {
        cout << "Khong the mo file: " << filename << "!\n";
        return listDoctors;
    }

    string line;
    getline(file, line); // Bỏ qua dòng tiêu đề
    getline(file, line); // Bỏ qua dòng phân cách ---+---

    while (getline(file, line)) {
        if (trimStr(line).empty()) continue;

        stringstream ss(line);
        string id, fullName, phone, genderStr, ageStr;
        string spId, stateStr, roomId, feeStr, ratingStr;

        getline(ss, id,        '|');
        getline(ss, fullName,  '|');
        getline(ss, phone,     '|');
        getline(ss, genderStr, '|');
        getline(ss, ageStr,    '|');
        getline(ss, spId,      '|');
        getline(ss, stateStr,  '|');
        getline(ss, roomId,    '|');
        getline(ss, feeStr,    '|');
        getline(ss, ratingStr, '|');

        id        = trimStr(id);
        fullName  = trimStr(fullName);
        phone     = trimStr(phone);
        genderStr = trimStr(genderStr);
        ageStr    = trimStr(ageStr);
        spId      = trimStr(spId);
        stateStr  = trimStr(stateStr);
        roomId    = trimStr(roomId);
        feeStr    = trimStr(feeStr);
        ratingStr = trimStr(ratingStr);

        bool   gender = (stoi(genderStr) == 1);
        int    age    = stoi(ageStr);
        bool   state  = (stoi(stateStr) == 1);
        double fee    = stod(feeStr);
        double rating = stod(ratingStr);

        Doctor doc(id, fullName, phone, gender, age,
                   spId, state, roomId, fee, rating);
        listDoctors.insert(doc);  // Dùng insert() của LinkList
    }

    file.close();
    return listDoctors;
}