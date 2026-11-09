#include "SymptomPatient.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "libPBL2.h"
#include "../Utils/AppExceptions.h"

using namespace std;

SymptomPatient::SymptomPatient()
    : SP_id(""), P_id(""), Sp_id(""), date(0, 0, 0, 0, 0) {}

SymptomPatient::SymptomPatient(std::string SP_id, std::string P_id, std::string Sp_id, Date date)
    : SP_id(SP_id), P_id(P_id), Sp_id(Sp_id), date(date) {}

SymptomPatient::SymptomPatient(const SymptomPatient &SP)
    : SP_id(SP.SP_id), P_id(SP.P_id), Sp_id(SP.Sp_id), date(SP.date) {}

void SymptomPatient::setDefault() {
    SP_id = "";
    P_id = "";
    Sp_id = "";
    date = Date(0, 0, 0, 0, 0);
}

bool SymptomPatient::isDefault() const {
    return SP_id.empty() && P_id.empty();
}

string SymptomPatient::getSP_id() const { return SP_id; }
string SymptomPatient::getP_id() const { return P_id; }
string SymptomPatient::getSp_id() const { return Sp_id; }
Date SymptomPatient::getDate() const { return date; }

void SymptomPatient::setSP_id(const string &value) { this->SP_id = value; }
void SymptomPatient::setP_id(const string &value) { this->P_id = value; }
void SymptomPatient::setSp_id(const string &value) { this->Sp_id = value; }
void SymptomPatient::setDate(const Date &value) { this->date = value; }

string SymptomPatient::toString() const {
    return libPBL2::padRightUtf8(SP_id, 6) + " | "
         + libPBL2::padRightUtf8(P_id, 6) + " | "
         + libPBL2::padRightUtf8(Sp_id, 6) + " | "
         + libPBL2::padRightUtf8(date.toString(), 16);
}

SymptomPatient SymptomPatient::loadFromString(const string &str) {
    if (str.empty()) return SymptomPatient();
    stringstream ss(str);
    string spId, pId, specId, dateStr;

    if (!getline(ss, spId, '|') ||
        !getline(ss, pId, '|') ||
        !getline(ss, specId, '|') ||
        !getline(ss, dateStr)) {
        return SymptomPatient();
    }

    libPBL2::trim(spId);
    libPBL2::trim(pId);
    libPBL2::trim(specId);
    libPBL2::trim(dateStr);

    if (spId.empty() || pId.empty())
        return SymptomPatient();

    Date d = Date::loadFromString(dateStr);
    return SymptomPatient(spId, pId, specId, d);
}

LinkList<SymptomPatient>* SymptomPatient::loadFromFile(const string &path) {
    ifstream f(path);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để đọc dữ liệu.");
    }

    LinkList<SymptomPatient> *SPList = nullptr;
    try {
        SPList = new LinkList<SymptomPatient>;
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
                SymptomPatient SP = SymptomPatient::loadFromString(line);
                if (!SP.isDefault()) {
                    SPList->insert(SP);
                }
            } catch (const AppException &) {
                throw;
            } catch (const std::exception &e) {
                throw DataFormatException(path, lineNo, e.what());
            }
        }

        f.close();
        return SPList;
    } catch (...) {
        delete SPList;
        f.close();
        throw;
    }
}

bool SymptomPatient::saveToFile(const string &path, const LinkList<SymptomPatient> *list) {
    if (!list || list->getHeader() == nullptr)
        return false;

    ofstream f(path);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để ghi dữ liệu.");
    }

    f << "SP_ID  | P_ID   | Sp_ID  | Date            \n";
    f << "-------+--------+--------+-----------------\n";

    LinkList<SymptomPatient>::Node *tmp = list->getHeader()->next;
    while (tmp != nullptr && tmp->next != nullptr) {
        if (!tmp->info.isDefault()) {
            f << tmp->info.toString() << "\n";
        }
        tmp = tmp->next;
    }

    f.close();
    return true;
}

string SymptomPatient::generateNextId(const LinkList<SymptomPatient> *list) {
    int maxId = 0;
    if (list && list->getHeader()) {
        LinkList<SymptomPatient>::Node *cur = list->getHeader()->next;
        while (cur != nullptr && cur->next != nullptr) {
            string id = cur->info.getSP_id();
            if (id.rfind("SP_", 0) == 0 && id.length() > 3) {
                try {
                    int num = stoi(id.substr(3));
                    if (num > maxId) maxId = num;
                } catch (...) {}
            }
            cur = cur->next;
        }
    }
    ostringstream oss;
    oss << "SP_" << setfill('0') << setw(3) << (maxId + 1);
    return oss.str();
}
