#include "Patient.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "libPBL2.h"
#include "../Utils/AppExceptions.h"

using namespace std;

Patient::Patient()
    : Human(), CCCD(""), state(1) {}

Patient::Patient(std::string id, std::string fullName, std::string phone, bool gender, int age, std::string CCCD, int state)
    : Human(id, fullName, phone, gender, age), CCCD(CCCD), state(state) {}

Patient::Patient(const Patient &P)
    : Human(P.id, P.fullName, P.phone, P.gender, P.age), CCCD(P.CCCD), state(P.state) {}


void Patient::setDefault() {
    Patient tmp;
    *this = tmp;
}

bool Patient::isDefault() const {
    return id == ""
        && fullName == ""
        && phone == ""
        && gender == 0
        && age == 0
        && CCCD == "";
}

string Patient::getCCCD() const { return this->CCCD; }
int Patient::getState() const { return this->state; }
void Patient::setCCCD(const string &CCCD) { this->CCCD = CCCD; }
void Patient::setState(int state) { this->state = state; }

string Patient::toString() const {
    return libPBL2::padRightUtf8(id, 5) + " | "
         + libPBL2::padRightUtf8(fullName, 23) + " | "
         + libPBL2::padRightUtf8(phone, 10) + " | "
         + libPBL2::padRightUtf8(gender ? "1" : "0", 8) + " | "
         + libPBL2::padRightUtf8(to_string(age), 5) + " | "
         + libPBL2::padRightUtf8(CCCD, 13) + " | "
         + to_string(state);
}

Patient Patient::loadFromString(const std::string &str) {
    if (str.empty()) return Patient();
    stringstream ss(str);
    string id, fullName, phone, genderStr, ageStr, cccd, stateStr;

    if (!getline(ss, id, '|') ||
        !getline(ss, fullName, '|') ||
        !getline(ss, phone, '|') ||
        !getline(ss, genderStr, '|') ||
        !getline(ss, ageStr, '|') ||
        !getline(ss, cccd, '|')) {
        // Trường hợp file cũ chỉ có 6 trường
        stringstream ss6(str);
        if (!getline(ss6, id, '|') ||
            !getline(ss6, fullName, '|') ||
            !getline(ss6, phone, '|') ||
            !getline(ss6, genderStr, '|') ||
            !getline(ss6, ageStr, '|') ||
            !getline(ss6, cccd)) {
            return Patient();
        }
        stateStr = "1";
    } else {
        getline(ss, stateStr);
    }

    libPBL2::trim(id);
    libPBL2::trim(fullName);
    libPBL2::trim(phone);
    libPBL2::trim(genderStr);
    libPBL2::trim(ageStr);
    libPBL2::trim(cccd);
    libPBL2::trim(stateStr);

    if (id.empty()) return Patient();

    bool gender = (genderStr == "1");
    int age = libPBL2::safeParseInt(ageStr, "Tuổi bệnh nhân", 0);
    int state = libPBL2::safeParseInt(stateStr, "Trạng thái bệnh nhân", 1);

    return Patient(id, fullName, phone, gender, age, cccd, state);
}

LinkList<Patient>* Patient::loadFromFile(const std::string &path) {
    ifstream f(path);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để đọc dữ liệu.");
    }

    LinkList<Patient> *patientList = nullptr;
    try {
        patientList = new LinkList<Patient>;
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
                Patient P = Patient::loadFromString(line);
                if (!P.isDefault()) {
                    patientList->insert(P);
                }
            } catch (const AppException &e) {
                throw DataFormatException(path, lineNo, e.what());
            } catch (const std::exception &e) {
                throw DataFormatException(path, lineNo, e.what());
            }
        }
        f.close();
        return patientList;
    } catch (...) {
        delete patientList;
        f.close();
        throw;
    }
}

bool Patient::saveToFile(const std::string &path, const LinkList<Patient> *list) {
    if (!list || list->getHeader() == nullptr)
        return false;

    string tempPath = path + ".tmp";
    ofstream f(tempPath);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp tạm để ghi dữ liệu.");
    }

    try {
        f << "P_ID  | P_FullName              | P_Phone    | P_Gender | P_Age | P_CCCD        | P_State\n";
        f << "------+-------------------------+------------+----------+-------+---------------+--------\n";

        LinkList<Patient>::Node *tmp = list->getHeader()->next;
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

string Patient::generateNextId(const LinkList<Patient> *list) {
    int maxId = 0;
    if (list && list->getHeader()) {
        LinkList<Patient>::Node *cur = list->getHeader()->next;
        while (cur != nullptr && cur->next != nullptr) {
            string idStr = cur->info.getID();
            if (idStr.length() > 2 && idStr.substr(0, 2) == "P_") {
                try {
                    int num = stoi(idStr.substr(2));
                    if (num > maxId) maxId = num;
                } catch (...) {}
            }
            cur = cur->next;
        }
    }
    ostringstream oss;
    oss << "P_" << setfill('0') << setw(3) << (maxId + 1);
    return oss.str();
}
