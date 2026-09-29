#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include "Models/Doctor.h"
#include "Controllers/human.cpp"
#include "Controllers/Doctor.cpp"

using namespace std;

// Tự động lấy đường dẫn gốc project từ vị trí file source lúc compile
// Hoạt động trên mọi máy mà không cần hardcode path
string getProjectRoot() {
    string path = __FILE__;              // VD: e:/PBL_2/main.cpp
    // Tìm vị trí của dấu '/' hoặc '\\' cuối cùng
    size_t pos = path.find_last_of("/\\");
    return (pos == string::npos) ? "." : path.substr(0, pos);
}

// Hàm trim khoảng trắng đầu/cuối
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}

// Hàm đọc file để test
vector<Doctor> loadDoctorsFromFile(string filename) {
    vector<Doctor> listDoctors;
    ifstream file(filename);
    
    if (!file.is_open()) {
        cout << "Khong the mo file " << filename << "!\n";
        return listDoctors;
    }

    string line;
    getline(file, line); // Bỏ qua dòng tiêu đề (header)
    getline(file, line); // Bỏ qua dòng phân cách ---+---

    while (getline(file, line)) {
        if (trim(line).empty()) continue; // Bỏ qua dòng trống

        stringstream ss(line);
        string id, fullName, phone, genderStr, ageStr, spId, stateStr, roomId, feeStr, ratingStr;
        
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

        // Trim khoảng trắng thừa
        id        = trim(id);
        fullName  = trim(fullName);
        phone     = trim(phone);
        genderStr = trim(genderStr);
        ageStr    = trim(ageStr);
        spId      = trim(spId);
        stateStr  = trim(stateStr);
        roomId    = trim(roomId);
        feeStr    = trim(feeStr);
        ratingStr = trim(ratingStr);

        // Ép kiểu dữ liệu
        bool gender = (stoi(genderStr) == 1);
        int age     = stoi(ageStr);
        bool state  = (stoi(stateStr) == 1);
        double fee    = stod(feeStr);
        double rating = stod(ratingStr);

        Doctor doc(id, fullName, phone, gender, age, spId, state, roomId, fee, rating);
        listDoctors.push_back(doc);
    }

    file.close();
    return listDoctors;
}

int main() {
    cout << "=== 1. TEST KHOI TAO TRUC TIEP DOCTOR ==-\n";
    Doctor testDoc("D_999", "Nguyen Van A", "0123456789", true, 40, "SP_99", true, "Room_999", 300000.0, 4.8);
    testDoc.displayInfo();

    cout << "\n=== 2. TEST DOC DU LIEU TU FILE DOCTORS.TXT ==-\n";
    string doctorFile = getProjectRoot() + "/Data/Doctor/Doctor.txt";
    vector<Doctor> doctors = loadDoctorsFromFile(doctorFile);
    
    cout << "Da doc thanh cong " << doctors.size() << " bac si tu file:\n";
    for (size_t i = 0; i < doctors.size(); ++i) {
        cout << "\n--- Bac si thu " << i + 1 << " ---\n";
        doctors[i].displayInfo();
    }

    return 0;
}