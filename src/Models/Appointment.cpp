#include "Appointment.h"
#include "libPBL2.h"
#include "../Utils/AppExceptions.h"
#include <fstream>
#include <string>
#include <cmath>
#include <cstdio>

Appointment::Appointment(std::string id, std::string symptomPatientId, std::string doctorId, const Date &dateGet, int state, double paidFee)
    : id(id), symptomPatientId(symptomPatientId), doctorId(doctorId), dateGet(dateGet), state(state), paidFee(paidFee) {}

std::string Appointment::getId() const { return id; }
std::string Appointment::getSymptomPatientId() const { return symptomPatientId; }
std::string Appointment::getDoctorId() const { return doctorId; }
Date Appointment::getDateGet() const { return dateGet; }
int Appointment::getState() const { return state; }
double Appointment::getPaidFee() const { return paidFee; }
bool Appointment::hasPaidFee() const { return paidFee >= 0.0; }

void Appointment::setId(const std::string &value) { id = value; }
void Appointment::setSymptomPatientId(const std::string &value) { symptomPatientId = value; }
void Appointment::setDoctorId(const std::string &value) { doctorId = value; }
void Appointment::setDateGet(const Date &value) { dateGet = value; }
void Appointment::setState(int value) { state = value; }
void Appointment::setPaidFee(double value) { paidFee = value; }

int Appointment::computeEffectiveState(int curDay, int curMonth, int curYear, int curHour, int curMin) const {
    if (state == -2) return -2;
    if (state == 1) return 1;
    if (state == -1) return -1;

    Date ad = getDateGet();
    if (ad.getYear() < curYear ||
       (ad.getYear() == curYear && ad.getMonth() < curMonth) ||
       (ad.getYear() == curYear && ad.getMonth() == curMonth && ad.getDay() < curDay)) {
        return -1;
    }
    if (ad.getYear() == curYear && ad.getMonth() == curMonth && ad.getDay() == curDay) {
        int apptMinutes = ad.getHour() * 60 + ad.getMinute();
        int curMinutes = curHour * 60 + curMin;
        if (apptMinutes + 20 < curMinutes) {
            return -1;
        }
    }
    return 0;
}

bool Appointment::isDefault() const {
    return id.empty() && symptomPatientId.empty() && doctorId.empty();
}

void Appointment::setDefault() {
    id.clear();
    symptomPatientId.clear();
    doctorId.clear();
    dateGet = Date();
    state = 0;
    paidFee = -1.0;
}

std::string Appointment::toString() const {
    std::string stStr = std::to_string(state);
    if (stStr.length() < 7) {
        stStr = std::string(7 - stStr.length(), ' ') + stStr;
    }
    std::string feeStr = "-";
    if (state == 1 && paidFee >= 0.0) {
        char buf[32];
        if (std::fabs(paidFee - std::round(paidFee)) < 1e-6) {
            std::snprintf(buf, sizeof(buf), "%.0f", std::round(paidFee));
        } else {
            std::snprintf(buf, sizeof(buf), "%.2f", paidFee);
        }
        feeStr = buf;
    }
    return libPBL2::padRightUtf8(id, 5) + " | "
         + libPBL2::padRightUtf8(symptomPatientId, 6) + " | "
         + libPBL2::padRightUtf8(doctorId, 5) + " | "
         + libPBL2::padRightUtf8(dateGet.toString(), 16) + " | "
         + stStr + " | "
         + libPBL2::padRightUtf8(feeStr, 10);
}

Appointment Appointment::loadFromString(const std::string &str) {
    if (str.empty()) return Appointment();
    std::string parts[8];
    int partCount = 0;
    size_t pos = 0;
    while (partCount < 8) {
        size_t bar = str.find('|', pos);
        std::string token = (bar == std::string::npos) ? str.substr(pos) : str.substr(pos, bar - pos);
        libPBL2::trim(token);
        parts[partCount++] = token;
        if (bar == std::string::npos) break;
        pos = bar + 1;
    }

    if (partCount < 5) {
        return Appointment();
    }

    std::string id = parts[0];
    std::string spId = parts[1];
    std::string doctorId = parts[2];
    std::string dateGetStr;
    std::string stateStr;
    double feeVal = -1.0;

    if (partCount >= 6) {
        bool col4LooksLikeDate = (parts[4].find('-') != std::string::npos && parts[4].find(':') != std::string::npos);
        if (col4LooksLikeDate) {
            dateGetStr = parts[4];
            stateStr = parts[5];
        } else {
            dateGetStr = parts[3];
            stateStr = parts[4];
            if (!parts[5].empty() && parts[5] != "-") {
                feeVal = libPBL2::safeParseDouble(parts[5], "Chi phí khám", -1.0);
            }
        }
    } else {
        dateGetStr = parts[3];
        stateStr = parts[4];
    }

    if (id.empty() || spId.empty() || doctorId.empty())
        return Appointment();

    Date dGet = Date::loadFromString(dateGetStr);
    int st = libPBL2::safeParseInt(stateStr, "Trạng thái cuộc hẹn", 0);
    if (st != 1) {
        feeVal = -1.0;
    }

    return Appointment(id, spId, doctorId, dGet, st, feeVal);
}

LinkList<Appointment>* Appointment::loadFromFile(const std::string &path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp để đọc dữ liệu.");
    }

    LinkList<Appointment> *list = nullptr;
    try {
        list = new LinkList<Appointment>;
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
                Appointment app = Appointment::loadFromString(line);
                if (!app.isDefault()) {
                    list->insert(app);
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
        return list;
    } catch (...) {
        delete list;
        f.close();
        throw;
    }
}

bool Appointment::saveToFile(const std::string &path, const LinkList<Appointment> *list) {
    if (!list || !list->getHeader()) return false;

    const std::string tempPath = path + ".tmp";
    std::ofstream f(tempPath);
    if (!f.is_open()) {
        throw FileIOException(path, "Không thể mở tệp tạm để ghi dữ liệu.");
    }

    try {
        f << "A_ID  | SP_ID  | D_ID  | A_DateGet        | A_State | A_Fee\n";
        f << "------+--------+-------+------------------+---------+-----------\n";

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
            throw FileIOException(path, "Lỗi ghi dữ liệu cuộc hẹn xuống tệp tạm.");
        }
        libPBL2::safeAtomicCommit(tempPath, path);
        return true;
    } catch (...) {
        f.close();
        std::remove(tempPath.c_str());
        throw;
    }
}
