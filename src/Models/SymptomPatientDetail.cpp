#include "SymptomPatientDetail.h"
#include <string>
#include <fstream>
#include <sstream>
#include "libPBL2.h"
#include "../Utils/AppExceptions.h"

using namespace std;

SymptomPatientDetail::SymptomPatientDetail()
    : SP_id(""), S_id("") {}

SymptomPatientDetail::SymptomPatientDetail(std::string SP_id, std::string S_id)
    : SP_id(SP_id), S_id(S_id) {}

SymptomPatientDetail::SymptomPatientDetail(const SymptomPatientDetail &other)
    : SP_id(other.SP_id), S_id(other.S_id) {}

void SymptomPatientDetail::setDefault() {
    SP_id = "";
    S_id = "";
}

bool SymptomPatientDetail::isDefault() const {
    return SP_id.empty() && S_id.empty();
}

string SymptomPatientDetail::getSP_id() const { return SP_id; }
string SymptomPatientDetail::getS_id() const { return S_id; }

void SymptomPatientDetail::setSP_id(const string &val) { this->SP_id = val; }
void SymptomPatientDetail::setS_id(const string &val) { this->S_id = val; }

string SymptomPatientDetail::toString() const {
    return libPBL2::padRightUtf8(SP_id, 6) + " | "
         + libPBL2::padRightUtf8(S_id, 6);
}

SymptomPatientDetail SymptomPatientDetail::loadFromString(const string &str) {
    if (str.empty()) return SymptomPatientDetail();
    stringstream ss(str);
    string spId, sId;

    if (!getline(ss, spId, '|') ||
        !getline(ss, sId)) {
        return SymptomPatientDetail();
    }

    libPBL2::trim(spId);
    libPBL2::trim(sId);

    if (spId.empty() || sId.empty())
        return SymptomPatientDetail();

    return SymptomPatientDetail(spId, sId);
}

LinkList<SymptomPatientDetail>* SymptomPatientDetail::loadFromFile(const string &path) {
    ifstream f(path);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để đọc dữ liệu.");
    }

    LinkList<SymptomPatientDetail> *list = nullptr;
    try {
        list = new LinkList<SymptomPatientDetail>;
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
                SymptomPatientDetail item = SymptomPatientDetail::loadFromString(line);
                if (!item.isDefault()) {
                    list->insert(item);
                }
            } catch (const AppException &) {
                throw;
            } catch (const std::exception &e) {
                throw DataFormatException(path, lineNo, e.what());
            }
        }

        f.close();
        return list;
    } catch (...) {
        delete list;
        f.close();
        throw;
    }
}

bool SymptomPatientDetail::saveToFile(const string &path, const LinkList<SymptomPatientDetail> *list) {
    if (!list || list->getHeader() == nullptr)
        return false;

    ofstream f(path);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để ghi dữ liệu.");
    }

    f << "SP_ID  | S_ID  \n";
    f << "-------+-------\n";

    LinkList<SymptomPatientDetail>::Node *tmp = list->getHeader()->next;
    while (tmp != nullptr && tmp->next != nullptr) {
        if (!tmp->info.isDefault()) {
            f << tmp->info.toString() << "\n";
        }
        tmp = tmp->next;
    }

    f.close();
    return true;
}
