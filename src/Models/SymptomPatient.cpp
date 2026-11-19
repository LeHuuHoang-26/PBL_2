#include "SymptomPatient.h"
#include <string>
#include <fstream>
#include <sstream>
#include "libPBL2.h"
#include "../Utils/AppExceptions.h"

SymptomPatient::SymptomPatient()
    : SP_id(""), P_id(""), Sp_id(""), date(0, 0, 0, 0, 0) {}

SymptomPatient::SymptomPatient(std::string SP_id, std::string P_id, std::string Sp_id, Date date)
    : SP_id(SP_id), P_id(P_id), Sp_id(Sp_id), date(date) {}

SymptomPatient::SymptomPatient(const SymptomPatient &SP)
    : SP_id(SP.SP_id), P_id(SP.P_id), Sp_id(SP.Sp_id), date(SP.date) {}

void SymptomPatient::setDefault() {
    SP_id.clear();
    P_id.clear();
    Sp_id.clear();
    date = Date(0, 0, 0, 0, 0);
}

bool SymptomPatient::isDefault() const {
    return SP_id.empty() && P_id.empty();
}

std::string SymptomPatient::getSP_id() const { return SP_id; }
std::string SymptomPatient::getP_id() const { return P_id; }
std::string SymptomPatient::getSp_id() const { return Sp_id; }
Date SymptomPatient::getDate() const { return date; }

void SymptomPatient::setSP_id(const std::string &value) { SP_id = value; }
void SymptomPatient::setP_id(const std::string &value) { P_id = value; }
void SymptomPatient::setSp_id(const std::string &value) { Sp_id = value; }
void SymptomPatient::setDate(const Date &value) { date = value; }

std::string SymptomPatient::toString() const {
    return libPBL2::padRightUtf8(SP_id, 6) + " | "
         + libPBL2::padRightUtf8(P_id, 6) + " | "
         + libPBL2::padRightUtf8(Sp_id, 6) + " | "
         + libPBL2::padRightUtf8(date.toString(), 16);
}

SymptomPatient SymptomPatient::loadFromString(const std::string &str) {
    if (str.empty()) return SymptomPatient();
    std::stringstream ss(str);
    std::string spId, pId, specId, dateStr;

    if (!std::getline(ss, spId, '|') ||
        !std::getline(ss, pId, '|') ||
        !std::getline(ss, specId, '|') ||
        !std::getline(ss, dateStr)) {
        return SymptomPatient();
    }

    libPBL2::trim(spId);
    libPBL2::trim(pId);
    libPBL2::trim(specId);
    libPBL2::trim(dateStr);

    if (spId.empty() || pId.empty()) {
        return SymptomPatient();
    }

    Date d = Date::loadFromString(dateStr);
    return SymptomPatient(spId, pId, specId, d);
}

LinkList<SymptomPatient>* SymptomPatient::loadFromFile(const std::string &path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để đọc dữ liệu.");
    }

    LinkList<SymptomPatient> *SPList = nullptr;
    try {
        SPList = new LinkList<SymptomPatient>;
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
                auto SP = SymptomPatient::loadFromString(line);
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

bool SymptomPatient::saveToFile(const std::string &path, const LinkList<SymptomPatient> *list) {
    if (!list || !list->getHeader()) return false;

    const std::string tempPath = path + ".tmp";
    std::ofstream f(tempPath);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp tạm để ghi dữ liệu.");
    }

    try {
        f << "SP_ID  | P_ID   | Sp_ID  | Date            \n";
        f << "-------+--------+--------+-----------------\n";

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
            throw FileIOException(path, "Lỗi ghi dữ liệu hồ sơ triệu chứng xuống tệp tạm.");
        }
        libPBL2::safeAtomicCommit(tempPath, path);
        return true;
    } catch (...) {
        f.close();
        std::remove(tempPath.c_str());
        throw;
    }
}

std::string SymptomPatient::generateNextId(const LinkList<SymptomPatient> *list) {
    return libPBL2::generateNextId(list, "SP_", [](const SymptomPatient &sp) { return sp.getSP_id(); });
}
