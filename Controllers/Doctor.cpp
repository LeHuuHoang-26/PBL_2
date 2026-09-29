#include "doctor.h"
#include <iostream>

using namespace std;

// Định nghĩa Constructor (Lưu ý: KHÔNG ghi lại các giá trị mặc định = "" ở đây)
Doctor::Doctor(string _id, string _lastName, string _middleName, 
               string _firstName, string _phoneNumbers, string _gender, 
               string _spId)
    : Human(_id, _lastName, _middleName, _firstName, _gender),
      phoneNumbers(_phoneNumbers), spId(_spId) {
}

// Định nghĩa Destructor
Doctor::~Doctor() {
    // Hiện tại dùng kiểu string chuẩn nên không cần giải phóng bộ nhớ thủ công
}

// Định nghĩa hàm hiển thị (Lưu ý: KHÔNG ghi lại từ khóa 'override' ở file .cpp)
void Doctor::displayInfo() const {
    cout << "==== THONG TIN BAC SI ====" << endl;
    Human::displayInfo(); 
    cout << "So dien thoai: " << phoneNumbers << endl;
    cout << "Ma chuyen khoa (Sp_ID): " << spId << endl;
    cout << "==========================" << endl;
}

// Định nghĩa các hàm Getter
string Doctor::getPhoneNumbers() const { 
    return phoneNumbers; 
}

string Doctor::getSpId() const { 
    return spId; 
}

// Định nghĩa các hàm Setter
void Doctor::setPhoneNumbers(string _phone) { 
    phoneNumbers = _phone; 
}

void Doctor::setSpId(string _spId) { 
    spId = _spId; 
}