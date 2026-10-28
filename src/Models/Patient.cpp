#include "Patient.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "libPBL2.h"

using namespace std;

Patient::Patient()
    : Human(), CCCD("") {}

Patient::Patient(std::ifstream &f) {
    string tmp;
    if (!getline(f, id, '|')) {
        setDefault();
        return;
    }
    libPBL2::trim(id);

    if (!getline(f, fullName, '|')) {
        setDefault();
        return;
    }
    libPBL2::trim(fullName);

    if (!getline(f, phone, '|')) {
        setDefault();
        return;
    }
    libPBL2::trim(phone);

    if (!getline(f, tmp, '|')) {
        setDefault();
        return;
    }
    gender = stoi(tmp);

    if (!getline(f, tmp, '|')) {
        setDefault();
        return;
    }
    age = stoi(tmp);

    string rest;
    if (!getline(f, rest, '\n')) {
        setDefault();
        return;
    }
    auto pipePos = rest.find('|');
    if (pipePos != string::npos) {
        CCCD = rest.substr(0, pipePos);
        string vStr = rest.substr(pipePos + 1);
        libPBL2::trim(vStr);
        valid = (vStr == "1");
    } else {
        CCCD = rest;
        valid = true;
    }
    libPBL2::trim(CCCD);
}

Patient::Patient(std::string id, std::string fullName, std::string phone, bool gender, int age, std::string CCCD, bool valid)
    : Human(id, fullName, phone, gender, age, valid), CCCD(CCCD) {}

Patient::Patient(const Patient &P)
    : Human(P.id, P.fullName, P.phone, P.gender, P.age, P.valid), CCCD(P.CCCD) {}

Patient::~Patient() {}

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
void Patient::setCCCD(string CCCD) { this->CCCD = CCCD; }

void Patient::save(std::ofstream &f) const {
    f << toString() << "\n";
}

string Patient::toString() const {
    return libPBL2::padRightUtf8(id, 5) + " | "
         + libPBL2::padRightUtf8(fullName, 23) + " | "
         + libPBL2::padRightUtf8(phone, 10) + " | "
         + libPBL2::padRightUtf8(gender ? "1" : "0", 8) + " | "
         + libPBL2::padRightUtf8(to_string(age), 5) + " | "
         + libPBL2::padRightUtf8(CCCD, 12) + " | "
         + (valid ? "1" : "0");
}

Patient Patient::loadFromString(const std::string &str) {
    if (str.empty()) return Patient();
    stringstream ss(str);
    string id, fullName, phone, genderStr, ageStr, cccd, validStr;

    if (!getline(ss, id, '|') ||
        !getline(ss, fullName, '|') ||
        !getline(ss, phone, '|') ||
        !getline(ss, genderStr, '|') ||
        !getline(ss, ageStr, '|')) {
        return Patient();
    }

    string rest;
    if (!getline(ss, rest)) {
        return Patient();
    }
    auto pipePos = rest.find('|');
    if (pipePos != string::npos) {
        cccd = rest.substr(0, pipePos);
        validStr = rest.substr(pipePos + 1);
    } else {
        cccd = rest;
        validStr = "1";
    }

    libPBL2::trim(id);
    libPBL2::trim(fullName);
    libPBL2::trim(phone);
    libPBL2::trim(genderStr);
    libPBL2::trim(ageStr);
    libPBL2::trim(cccd);
    libPBL2::trim(validStr);

    if (id.empty()) return Patient();

    bool gender = (genderStr == "1");
    int age = 0;
    try {
        age = stoi(ageStr);
    } catch (...) {}

    bool valid = validStr.empty() || (validStr == "1");
    return Patient(id, fullName, phone, gender, age, cccd, valid);
}

LinkList<Patient>* Patient::loadFromFile(const std::string &path) {
    LinkList<Patient> *patientList = new LinkList<Patient>;
    if (!patientList) return nullptr;

    ifstream f(path);
    if (!f.is_open()) {
        delete patientList;
        return nullptr;
    }

    string tmp;
    getline(f, tmp); // Bỏ qua tiêu đề
    getline(f, tmp); // Bỏ qua dòng phân cách

    while (true) {
        Patient P(f);
        if (P.isDefault())
            break;
        patientList->insert(P);
    }
    f.close();
    return patientList;
}

bool Patient::saveToFile(const std::string &path, const LinkList<Patient> *list) {
    if (!list || list->getHeader() == nullptr)
        return false;

    ofstream f(path);
    if (!f.is_open())
        return false;

    f << "P_ID  | P_FullName              | P_Phone    | P_Gender | P_Age | P_CCCD       | P_Valid\n";
    f << "------+-------------------------+------------+----------+-------+--------------+--------\n";

    LinkList<Patient>::Node *tmp = list->getHeader()->next;
    while (tmp != nullptr && tmp->next != nullptr) {
        if (!tmp->info.isDefault()) {
            tmp->info.save(f);
        }
        tmp = tmp->next;
    }

    f.close();
    return true;
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
