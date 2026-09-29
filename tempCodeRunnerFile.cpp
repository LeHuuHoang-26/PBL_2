#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include "Doctor.h"
#include "Doctor.cpp"

using namespace std;

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

    while (getline(file, line)) {
        stringstream ss(line);
        string id, fullName, phone, genderStr, ageStr, spId, stateStr, roomId, feeStr, ratingStr;
        
        getline(ss, id, '|');
        getline(ss, fullName, '|');
        getline(ss, phone, '|');
        getline(ss, genderStr, '|');
        getline(ss, ageStr, '|');
        getline(ss, spId, '|');
        getline(ss, stateStr, '|');
        getline(ss, roomId, '|');
        getline(ss, feeStr, '|');
        getline(ss, ratingStr, '|');

        // Ép kiểu dữ liệu an toàn cơ bản
        bool gender = (stoi(genderStr) == 1);
        int age = stoi(ageStr);
        bool state = (stoi(stateStr) == 1);
        double fee = stod(feeStr);
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
    vector<Doctor> doctors = loadDoctorsFromFile("doctors.txt");
    
    cout << "Da doc thanh cong " << doctors.size() << " bac si tu file:\n";
    for (size_t i = 0; i < doctors.size(); ++i) {
        cout << "\n--- Bac si thu " << i + 1 << " ---\n";
        doctors[i].displayInfo();
    }

    return 0;
}