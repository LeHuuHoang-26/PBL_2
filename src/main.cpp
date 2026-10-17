#include <iostream>
#include <string>
#include "Doctor.h"
#include "Patient.h"
#include "Manager.h"
#include "Appointment.h"
#include "QueuePatient.h"
#include "Symptom.h"
#include "Date.h"
#include "LinkList.h"
#include "Queue.h"
#include "libPBL2.h"

using namespace std;

// In dòng phân cách cho đẹp
void printHeader(const string &title) {
    cout << "\n======================================================\n";
    cout << "  " << title << "\n";
    cout << "======================================================\n";
}

int main() {
    // Thiết lập hiển thị UTF-8 trên Windows Console
    system("chcp 65001 > nul");

    cout << "\n>>> BAT DAU KIEM TRA TOAN BO SOURCE CODE DU AN PBL2 <<<\n";

    // ---------------------------------------------------------------
    // TEST 1: Cấu trúc dữ liệu tự cài đặt (DataStructures)
    // ---------------------------------------------------------------
    printHeader("TEST 1: CAU TRUC DU LIEU TU CAI DAT (LinkList & Queue)");
    {
        // 1.1 LinkList
        cout << "[1.1] Kiem tra LinkList<string>:\n";
        LinkList<string> list;
        list.insert("Node 1: PBL2");
        list.insert("Node 2: Quan ly phong kham");
        list.insert("Node 3: Cu Phap C++23");

        cout << "  - So luong phan tu trong list: " << list.count << "\n";
        cout << "  - Cac phan tu theo thu tu: \n";
        auto *cur = list.getHeader()->next;
        int idx = 0;
        while (cur != list.getFooter()) {
            cout << "      " << ++idx << ". " << cur->info << "\n";
            cur = cur->next;
        }

        // 1.2 Queue
        cout << "\n[1.2] Kiem tra Queue<int> (FIFO):\n";
        Queue<int> q;
        q.enQueue(101);
        q.enQueue(202);
        q.enQueue(303);
        cout << "  - Enqueue 3 gia tri: 101, 202, 303\n";

        cout << "  - Dequeue lan luot: ";
        while (!q.isEmpty()) {
            int *val = q.deQueue();
            if (val) {
                cout << *val << " ";
                delete val;
            }
        }
        cout << "\n  - Trang thai Queue sau khi dequeue het: " << (q.isEmpty() ? "RONG (Dung)" : "Loi") << "\n";
    }

    // ---------------------------------------------------------------
    // TEST 2: Tiện ích Date & libPBL2 (Utils)
    // ---------------------------------------------------------------
    printHeader("TEST 2: TIEN ICH (Date & libPBL2)");
    {
        Date d1(17, 10, 2026, 19, 30);
        cout << "  - Khoi tao Date thu cong: " << d1.toString() << "\n";

        Date d2 = Date::loadFromString("2026-12-25 08:45");
        cout << "  - Parse Date tu chuoi '2026-12-25 08:45': " << d2.toString() << "\n";

        string rawText = "   Xin chao PBL2   ";
        libPBL2::trim(rawText);
        cout << "  - Test libPBL2::trim: '" << rawText << "'\n";

        string padded = libPBL2::padRightUtf8("Bác Sĩ", 15);
        cout << "  - Test padRightUtf8: [" << padded << "](width 15)\n";
    }

    // ---------------------------------------------------------------
    // TEST 3: Entity Models - Doctor & Patient
    // ---------------------------------------------------------------
    printHeader("TEST 3: DOC DU LIEU DOCTOR & PATIENT TU CSDL TEXT");
    {
        // 3.1 Doctor
        string docPath = libPBL2::getDataPath("Doctor/Doctor.txt");
        LinkList<Doctor> *doctors = Doctor::loadFromFile(docPath);
        if (doctors) {
            cout << "[3.1] Doc thanh cong file Doctor.txt:\n";
            cout << "  - Tong so bac si: " << doctors->count << "\n";
            cout << "  - Thong tin bac si dau tien trong file:\n";
            if (doctors->getHeader()->next != doctors->getFooter()) {
                doctors->getHeader()->next->info.displayInfo();
            }
            delete doctors;
        } else {
            cout << "[3.1] LOI: Khong the doc file " << docPath << "\n";
        }

        // 3.2 Patient
        string patientPath = libPBL2::getDataPath("Patient.txt");
        LinkList<Patient> *patients = Patient::loadFromFile(patientPath);
        if (patients) {
            cout << "\n[3.2] Doc thanh cong file Patient.txt:\n";
            cout << "  - Tong so benh nhan: " << patients->count << "\n";
            cout << "  - Thong tin benh nhan dau tien trong file:\n";
            if (patients->getHeader()->next != patients->getFooter()) {
                patients->getHeader()->next->info.displayInfo();
            }
            delete patients;
        } else {
            cout << "[3.2] LOI: Khong the doc file " << patientPath << "\n";
        }
    }

    // ---------------------------------------------------------------
    // TEST 4: Manager & Xac thuc tai khoan (Auth)
    // ---------------------------------------------------------------
    printHeader("TEST 4: TAI KHOAN QUAN LY & XAC THUC (Manager)");
    {
        string managerPath = libPBL2::getDataPath("Manager.txt");
        LinkList<Manager> *managers = Manager::loadFromFile(managerPath);
        if (managers) {
            cout << "  - Tong so tai khoan Manager: " << managers->count << "\n";

            // Kiem tra thu dang nhap
            string testUser = "HoangMA001";
            string testPass = "HoangMA001";
            string wrongPass = "SaiMatKhau123";

            Manager *found = nullptr;
            auto *node = managers->getHeader()->next;
            while (node != managers->getFooter()) {
                if (node->info.getUsername() == testUser) {
                    found = &node->info;
                    break;
                }
                node = node->next;
            }

            if (found) {
                cout << "  - Tim thay user '" << testUser << "': " << found->getfullName() << "\n";
                cout << "  - Test login dung pass ('" << testPass << "'): " 
                     << (found->isValid(testUser, testPass) ? "DANG NHAP THANH CONG" : "THAT BAI") << "\n";
                cout << "  - Test login sai pass ('" << wrongPass << "'): " 
                     << (found->isValid(testUser, wrongPass) ? "LOI" : "CHAN DANG NHAP THANH CONG") << "\n";
            }
            delete managers;
        } else {
            cout << "  - LOI: Khong the doc file " << managerPath << "\n";
        }
    }

    // ---------------------------------------------------------------
    // TEST 5: Appointment (Lich hen) & QueuePatient (Hang doi kham)
    // ---------------------------------------------------------------
    printHeader("TEST 5: LICH HEN (Appointment) & HANG DOI KHAM (QueuePatient)");
    {
        // 5.1 Appointment
        string appPath = libPBL2::getDataPath("Appointment/Appointment.txt");
        LinkList<Appointment> *apps = Appointment::loadFromFile(appPath);
        if (apps) {
            cout << "[5.1] Doc thanh cong Appointment.txt:\n";
            cout << "  - Tong so lich hen: " << apps->count << "\n";
            if (apps->getHeader()->next != apps->getFooter()) {
                cout << "  - Thong tin lich hen dau tien:\n";
                apps->getHeader()->next->info.displayInfo();
            }
            delete apps;
        }

        // 5.2 QueuePatient
        string qpPath = libPBL2::getDataPath("QueuePatient.txt");
        Queue<QueuePatient> *queuePatient = QueuePatient::loadFromFile(qpPath);
        if (queuePatient) {
            cout << "\n[5.2] Doc thanh cong QueuePatient.txt vao Queue:\n";
            auto *firstWaiting = queuePatient->getFirstWaiting();
            if (firstWaiting && firstWaiting->data) {
                cout << "  - Benh nhan dau tien dang cho kham (State = 0): " 
                     << firstWaiting->data->getQP_id() 
                     << " | Chuyen khoa: " << firstWaiting->data->getSp_id() << "\n";
            } else {
                cout << "  - Khong co benh nhan nao dang cho kham trong hang doi.\n";
            }
            delete queuePatient;
        }
    }

    cout << "\n======================================================\n";
    cout << "  >>> TAT CA CAC TEST DA HOAN TAT THANH CONG! <<<\n";
    cout << "======================================================\n";

    return 0;
}
