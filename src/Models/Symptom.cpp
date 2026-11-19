#include "Symptom.h"
#include <fstream>
#include <sstream>
#include "libPBL2.h"
#include "../Utils/AppExceptions.h"

Symptom::Symptom()
    : S_id(""), sName(""), Sp_id(""), sScore(0) {}

Symptom::Symptom(std::string S_id, std::string sName, std::string Sp_id, int sScore)
    : S_id(S_id), sName(sName), Sp_id(Sp_id), sScore(sScore) {}

Symptom::Symptom(const Symptom &S)
    : S_id(S.S_id), sName(S.sName), Sp_id(S.Sp_id), sScore(S.sScore) {}

void Symptom::setDefault() {
    S_id.clear();
    sName.clear();
    Sp_id.clear();
    sScore = 0;
}

bool Symptom::isDefault() const {
    return S_id.empty() && sName.empty() && Sp_id.empty() && (sScore == 0);
}

std::string Symptom::getS_id() const { return S_id; }
std::string Symptom::getSName() const { return sName; }
std::string Symptom::getSp_id() const { return Sp_id; }
int Symptom::getSScore() const { return sScore; }

void Symptom::setS_id(const std::string &id) { S_id = id; }
void Symptom::setSName(const std::string &name) { sName = name; }
void Symptom::setSp_id(const std::string &spId) { Sp_id = spId; }
void Symptom::setSScore(int score) { sScore = score; }

std::string Symptom::toString() const {
    return libPBL2::padRightUtf8(S_id, 6) + " | "
         + libPBL2::padRightUtf8(sName, 19) + " | "
         + libPBL2::padRightUtf8(Sp_id, 6) + " | "
         + std::to_string(sScore);
}

Symptom Symptom::loadFromString(const std::string &str) {
    if (str.empty()) return Symptom();
    std::stringstream ss(str);
    std::string sId, name, spId, scoreStr;

    if (!std::getline(ss, sId, '|') ||
        !std::getline(ss, name, '|') ||
        !std::getline(ss, spId, '|') ||
        !std::getline(ss, scoreStr)) {
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

LinkList<Symptom>* Symptom::loadFromFile(const std::string &path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để đọc dữ liệu.");
    }

    LinkList<Symptom> *symptomList = nullptr;
    try {
        symptomList = new LinkList<Symptom>;
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
                auto S = Symptom::loadFromString(line);
                if (!S.isDefault()) {
                    symptomList->insert(S);
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
        return symptomList;
    } catch (...) {
        delete symptomList;
        f.close();
        throw;
    }
}

bool Symptom::saveToFile(const std::string &path, const LinkList<Symptom> *list) {
    if (!list || !list->getHeader()) return false;

    const std::string tempPath = path + ".tmp";
    std::ofstream f(tempPath);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp tạm để ghi dữ liệu.");
    }

    try {
        f << "S_ID  | S_Name              | Sp_ID  | S_Score\n";
        f << "------+---------------------+--------+--------\n";

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
            throw FileIOException(path, "Lỗi ghi dữ liệu triệu chứng xuống tệp tạm.");
        }
        libPBL2::safeAtomicCommit(tempPath, path);
        return true;
    } catch (...) {
        f.close();
        std::remove(tempPath.c_str());
        throw;
    }
}
