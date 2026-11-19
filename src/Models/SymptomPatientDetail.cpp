#include "SymptomPatientDetail.h"
#include <string>
#include <fstream>
#include <sstream>
#include "libPBL2.h"
#include "../Utils/AppExceptions.h"

SymptomPatientDetail::SymptomPatientDetail()
    : SP_id(""), S_id("") {}

SymptomPatientDetail::SymptomPatientDetail(std::string SP_id, std::string S_id)
    : SP_id(SP_id), S_id(S_id) {}

SymptomPatientDetail::SymptomPatientDetail(const SymptomPatientDetail &other)
    : SP_id(other.SP_id), S_id(other.S_id) {}

void SymptomPatientDetail::setDefault() {
    SP_id.clear();
    S_id.clear();
}

bool SymptomPatientDetail::isDefault() const {
    return SP_id.empty() && S_id.empty();
}

std::string SymptomPatientDetail::getSP_id() const { return SP_id; }
std::string SymptomPatientDetail::getS_id() const { return S_id; }

void SymptomPatientDetail::setSP_id(const std::string &val) { SP_id = val; }
void SymptomPatientDetail::setS_id(const std::string &val) { S_id = val; }

std::string SymptomPatientDetail::toString() const {
    return libPBL2::padRightUtf8(SP_id, 6) + " | " + libPBL2::padRightUtf8(S_id, 6);
}

SymptomPatientDetail SymptomPatientDetail::loadFromString(const std::string &str) {
    if (str.empty()) return SymptomPatientDetail();
    std::stringstream ss(str);
    std::string spId, sId;

    if (!std::getline(ss, spId, '|') || !std::getline(ss, sId)) {
        return SymptomPatientDetail();
    }

    libPBL2::trim(spId);
    libPBL2::trim(sId);

    if (spId.empty() || sId.empty()) {
        return SymptomPatientDetail();
    }

    return SymptomPatientDetail(spId, sId);
}

LinkList<SymptomPatientDetail>* SymptomPatientDetail::loadFromFile(const std::string &path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để đọc dữ liệu.");
    }

    LinkList<SymptomPatientDetail> *list = nullptr;
    try {
        list = new LinkList<SymptomPatientDetail>;
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
                auto item = SymptomPatientDetail::loadFromString(line);
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

bool SymptomPatientDetail::saveToFile(const std::string &path, const LinkList<SymptomPatientDetail> *list) {
    if (!list || !list->getHeader()) return false;

    const std::string tempPath = path + ".tmp";
    std::ofstream f(tempPath);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp tạm để ghi dữ liệu.");
    }

    try {
        f << "SP_ID  | S_ID  \n";
        f << "-------+-------\n";

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
            throw FileIOException(path, "Lỗi ghi chi tiết triệu chứng xuống tệp tạm.");
        }
        libPBL2::safeAtomicCommit(tempPath, path);
        return true;
    } catch (...) {
        f.close();
        std::remove(tempPath.c_str());
        throw;
    }
}
