#include <iostream>
#include <string>
#include "Models/Doctor.h"
#include "Controllers/human.cpp"
#include "Controllers/Doctor.cpp"

using namespace std;

// Tự động lấy đường dẫn gốc project từ vị trí file source lúc compile
// Hoạt động trên mọi máy mà không cần hardcode path
string getProjectRoot() {
    string path = __FILE__;              // VD: e:/PBL_2/main.cpp
    size_t pos = path.find_last_of("/\\");
    return (pos == string::npos) ? "." : path.substr(0, pos);
}

int main() {
    cout << "=== 1. TEST KHOI TAO TRUC TIEP DOCTOR ==\n";
    Doctor testDoc("D_999", "Nguyen Van A", "0123456789", true, 40, "SP_99", true, "Room_999", 300000.0, 4.8);
    testDoc.displayInfo();

    cout << "\n=== 2. TEST DOC DU LIEU TU FILE DOCTORS.TXT ==\n";
    string doctorFile = getProjectRoot() + "/Data/Doctor/Doctor.txt";
    LinkList<Doctor> doctors = Doctor::loadFromFile(doctorFile);

    // Duyệt LinkList: từ header->next đến khi next == nullptr (footer)
    int count = 0;
    LinkList<Doctor>::Node* node = doctors.getHeader()->next;
    while (node != doctors.getFooter()) {
        cout << "\n--- Bac si thu " << ++count << " ---\n";
        node->info.displayInfo();
        node = node->next;
    }
    cout << "\nTong so bac si doc duoc: " << count << "\n";

    return 0;
}
