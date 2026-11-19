#include "Doctor.h"
#include "libPBL2.h"
#include "../Utils/AppExceptions.h"
#include <fstream>
#include <sstream>
#include <iomanip>

Doctor::Doctor(std::string _id, std::string _fullName, std::string _phone, bool _gender, int _age, std::string _spId, int _state, std::string _roomId, double _fee, double _rating)
    : Human(_id, _fullName, _phone, _gender, _age), spId(_spId), state(_state), roomId(_roomId), fee(_fee), rating(_rating) {}

bool Doctor::isDefault() const {
    return id.empty() && fullName.empty();
}

void Doctor::setDefault() {
    id.clear();
    fullName.clear();
    phone.clear();
    gender = true;
    age = 0;
    spId.clear();
    state = 1;
    roomId.clear();
    fee = 0.0;
    rating = 5.0;
}

std::string Doctor::getSpId() const { return spId; }
int Doctor::getState() const { return state; }
std::string Doctor::getRoomId() const { return roomId; }
double Doctor::getFee() const { return fee; }
double Doctor::getRating() const { return rating; }

void Doctor::setSpId(const std::string &_spId) { this->spId = _spId; }
void Doctor::setState(int _state) { this->state = _state; }
void Doctor::setRoomId(const std::string &_roomId) { this->roomId = _roomId; }
void Doctor::setFee(double _fee) { this->fee = _fee; }
void Doctor::setRating(double _rating) { this->rating = _rating; }

std::string Doctor::toString() const {
    std::ostringstream oss;
    oss << libPBL2::padRightUtf8(id, 6) << " | "
        << libPBL2::padRightUtf8(fullName, 20) << " | "
        << libPBL2::padRightUtf8(phone, 10) << " | "
        << libPBL2::padRightUtf8(gender ? "1" : "0", 8) << " | "
        << libPBL2::padRightUtf8(std::to_string(age), 5) << " | "
        << libPBL2::padRightUtf8(spId, 6) << " | "
        << libPBL2::padRightUtf8(std::to_string(state), 7) << " | "
        << libPBL2::padRightUtf8(roomId, 8) << " | "
        << libPBL2::padRightUtf8([&]() {
            std::ostringstream fss;
            fss << std::fixed << std::setprecision(1) << fee;
            return fss.str();
        }(), 16) << " | "
        << libPBL2::padRightUtf8([&]() {
            std::ostringstream rss;
            rss << std::fixed << std::setprecision(1) << rating;
            return rss.str();
        }(), 6);
    return oss.str();
}

Doctor Doctor::loadFromString(const std::string &str) {
    if (str.empty()) return Doctor();
    std::stringstream ss(str);
    std::string id, fullName, phone, genderStr, ageStr;
    std::string spId, stateStr, roomId, feeStr, ratingStr;

    if (!std::getline(ss, id, '|') ||
        !std::getline(ss, fullName, '|') ||
        !std::getline(ss, phone, '|') ||
        !std::getline(ss, genderStr, '|') ||
        !std::getline(ss, ageStr, '|') ||
        !std::getline(ss, spId, '|') ||
        !std::getline(ss, stateStr, '|') ||
        !std::getline(ss, roomId, '|') ||
        !std::getline(ss, feeStr, '|') ||
        !std::getline(ss, ratingStr)) {
        return Doctor();
    }

    libPBL2::trim(id);
    libPBL2::trim(fullName);
    libPBL2::trim(phone);
    libPBL2::trim(genderStr);
    libPBL2::trim(ageStr);
    libPBL2::trim(spId);
    libPBL2::trim(stateStr);
    libPBL2::trim(roomId);
    libPBL2::trim(feeStr);
    libPBL2::trim(ratingStr);

    if (id.empty()) return Doctor();

    bool gender = (genderStr == "1");
    int age = libPBL2::safeParseInt(ageStr, "Tuổi bác sĩ", 0);
    int state = libPBL2::safeParseInt(stateStr, "Trạng thái công tác", 1);
    double fee = libPBL2::safeParseDouble(feeStr, "Phí khám", 0.0);

    size_t pipePos = ratingStr.find('|');
    if (pipePos != std::string::npos) ratingStr = ratingStr.substr(0, pipePos);
    libPBL2::trim(ratingStr);
    double rating = libPBL2::safeParseDouble(ratingStr, "Đánh giá chuyên môn", 5.0);

    return Doctor(id, fullName, phone, gender, age, spId, state, roomId, fee, rating);
}

LinkList<Doctor>* Doctor::loadFromFile(const std::string &path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để đọc dữ liệu.");
    }

    LinkList<Doctor> *doctorList = nullptr;
    try {
        doctorList = new LinkList<Doctor>;
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
                Doctor doc = Doctor::loadFromString(line);
                if (!doc.isDefault()) {
                    doctorList->insert(doc);
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
        return doctorList;
    } catch (...) {
        delete doctorList;
        f.close();
        throw;
    }
}

bool Doctor::saveToFile(const std::string &path, const LinkList<Doctor> *list) {
    if (!list || !list->getHeader()) return false;

    const std::string tempPath = path + ".tmp";
    std::ofstream f(tempPath);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp tạm để ghi dữ liệu.");
    }

    try {
        f << "D_ID   | D_FullName           | D_Phone    | D_Gender | D_Age | Sp_ID  | D_State | Room_ID  | Consultation_Fee | Rating\n";
        f << "-------+----------------------+------------+----------+-------+--------+---------+----------+------------------+-------\n";

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
            throw FileIOException(path, "Lỗi ghi dữ liệu bác sĩ xuống tệp tạm.");
        }
        libPBL2::safeAtomicCommit(tempPath, path);
        return true;
    } catch (...) {
        f.close();
        std::remove(tempPath.c_str());
        throw;
    }
}
