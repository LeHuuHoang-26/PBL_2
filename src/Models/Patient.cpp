#include "Patient.h"
#include <string>
#include <fstream>
#include <sstream>
#include "libPBL2.h"
#include "AppExceptions.h"

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
    return id.empty()
        && fullName.empty()
        && phone.empty()
        && !gender
        && (age == 0)
        && CCCD.empty();
}

std::string Patient::getCCCD() const { return CCCD; }
int Patient::getState() const { return state; }

void Patient::setCCCD(const std::string &CCCD) { this->CCCD = CCCD; }
void Patient::setState(int state) { this->state = state; }

std::string Patient::toString() const {
    return libPBL2::padRightUtf8(id, 5) + " | "
         + libPBL2::padRightUtf8(fullName, 23) + " | "
         + libPBL2::padRightUtf8(phone, 10) + " | "
         + libPBL2::padRightUtf8(gender ? "1" : "0", 8) + " | "
         + libPBL2::padRightUtf8(std::to_string(age), 5) + " | "
         + libPBL2::padRightUtf8(CCCD, 13) + " | "
         + std::to_string(state);
}

Patient Patient::loadFromString(const std::string &str) {
    if (str.empty()) return Patient();
    std::stringstream ss(str);
    std::string id, fullName, phone, genderStr, ageStr, cccd, stateStr;

    if (!std::getline(ss, id, '|') ||
        !std::getline(ss, fullName, '|') ||
        !std::getline(ss, phone, '|') ||
        !std::getline(ss, genderStr, '|') ||
        !std::getline(ss, ageStr, '|') ||
        !std::getline(ss, cccd, '|')) {
        return Patient();
    }
    std::getline(ss, stateStr);

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
    std::ifstream f(path);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để đọc dữ liệu.");
    }

    LinkList<Patient> *patientList = nullptr;
    try {
        patientList = new LinkList<Patient>;
        std::string tmp;
        std::getline(f, tmp);
        std::getline(f, tmp);

        std::string line;
        int lineNo = 2;
        while (std::getline(f, line)) {
            ++lineNo;
            libPBL2::trim(line);
            if (line.empty()) continue;
            try {
                Patient P = Patient::loadFromString(line);
                if (!P.isDefault()) {
                    patientList->insert(P);
                }
            } catch (const MemoryAllocationException &) {
                throw;
            } catch (const DataFormatException &e) {
                throw DataFormatException(path, lineNo, e.what());
            } catch (const AppException &) {
                throw;
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
    if (!list || !list->getHeader()) return false;

    const std::string tempPath = path + ".tmp";
    std::ofstream f(tempPath);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp tạm để ghi dữ liệu.");
    }

    try {
        f << "P_ID  | P_FullName              | P_Phone    | P_Gender | P_Age | P_CCCD        | P_State\n";
        f << "------+-------------------------+------------+----------+-------+---------------+--------\n";

        for (auto *tmp = list->getHeader()->next; tmp && tmp->next; tmp = tmp->next) {
            if (!tmp->info.isDefault()) {
                f << tmp->info.toString() << '\n';
            }
        }

        f.flush();
        const bool writeOk = f.good();
        f.close();
        if (!writeOk || f.fail()) {
            std::remove(tempPath.c_str());
            throw FileIOException(path, "Lỗi ghi dữ liệu bệnh nhân xuống tệp tạm.");
        }
        libPBL2::safeAtomicCommit(tempPath, path);
        return true;
    } catch (...) {
        f.close();
        std::remove(tempPath.c_str());
        throw;
    }
}

std::string Patient::generateNextId(const LinkList<Patient> *list) {
    return libPBL2::generateNextId(list, "P_", [](const Patient &p) { return p.getID(); });
}
