#include "Symptom.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "libPBL2.h"
#include "../Utils/AppExceptions.h"

using namespace std;

Symptom::Symptom()
    : S_id(""), sName(""), Sp_id(""), sScore(0) {}

Symptom::Symptom(std::string S_id, std::string sName, std::string Sp_id, int sScore)
    : S_id(S_id), sName(sName), Sp_id(Sp_id), sScore(sScore) {}

Symptom::Symptom(const Symptom &S)
    : S_id(S.S_id), sName(S.sName), Sp_id(S.Sp_id), sScore(S.sScore) {}

void Symptom::setDefault() {
    S_id = "";
    sName = "";
    Sp_id = "";
    sScore = 0;
}

bool Symptom::isDefault() const {
    return S_id == "" && sName == "" && Sp_id == "" && sScore == 0;
}

string Symptom::getS_id() const { return S_id; }
string Symptom::getSName() const { return sName; }
string Symptom::getSp_id() const { return Sp_id; }
int Symptom::getSScore() const { return sScore; }

void Symptom::setS_id(string id) { S_id = id; }
void Symptom::setSName(string name) { sName = name; }
void Symptom::setSp_id(string spId) { Sp_id = spId; }
void Symptom::setSScore(int score) { sScore = score; }

string Symptom::toString() const {
    return libPBL2::padRightUtf8(S_id, 6) + " | "
         + libPBL2::padRightUtf8(sName, 19) + " | "
         + libPBL2::padRightUtf8(Sp_id, 6) + " | "
         + to_string(sScore);
}

Symptom Symptom::loadFromString(const string &str) {
    if (str.empty()) return Symptom();
    stringstream ss(str);
    string sId, name, spId, scoreStr;

    if (!getline(ss, sId, '|') ||
        !getline(ss, name, '|') ||
        !getline(ss, spId, '|') ||
        !getline(ss, scoreStr)) {
        return Symptom();
    }

    libPBL2::trim(sId);
    libPBL2::trim(name);
    libPBL2::trim(spId);
    libPBL2::trim(scoreStr);

    if (sId.empty()) return Symptom();
    int sc = libPBL2::safeParseInt(scoreStr, "Điểm triệu chứng", 0);

    return Symptom(sId, name, spId, sc);
}

LinkList<Symptom>* Symptom::loadFromFile(const string &path) {
    ifstream f(path);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để đọc dữ liệu.");
    }

    LinkList<Symptom> *symptomList = nullptr;
    try {
        symptomList = new LinkList<Symptom>;
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
                Symptom S = Symptom::loadFromString(line);
                if (!S.isDefault()) {
                    symptomList->insert(S);
                }
            } catch (const AppException &e) {
                throw DataFormatException(path, lineNo, e.what());
            } catch (const std::exception &e) {
                throw DataFormatException(path, lineNo, e.what());
            }
        }
        f.close();
        return symptomList;
    } catch (...) {
        delete symptomList;
        f.close();
        throw;
    }
}

bool Symptom::saveToFile(const string &path, const LinkList<Symptom> *list) {
    if (!list || list->getHeader() == nullptr)
        return false;

    string tempPath = path + ".tmp";
    ofstream f(tempPath);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp tạm để ghi dữ liệu.");
    }

    try {
        f << "S_ID  | S_Name              | Sp_ID  | S_Score\n";
        f << "------+---------------------+--------+--------\n";

        LinkList<Symptom>::Node *tmp = list->getHeader()->next;
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
