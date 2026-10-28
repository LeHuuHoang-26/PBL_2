#include "AppointmentController.h"
#include "Appointment.h"
#include "Date.h"
#include "libPBL2.h"
#include "UIHelper.h"
#include <QDateTime>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

int AppointmentController::count(const LinkList<Appointment> *appointments) {
    if (!appointments || !appointments->getHeader()) return 0;
    int c = 0;
    auto *n = appointments->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getValid()) c++;
        n = n->next;
    }
    return c;
}

int AppointmentController::pendingAppointmentCount(const LinkList<Appointment> *appointments) {
    if (!appointments || !appointments->getHeader()) return 0;
    int c = 0;
    auto *n = appointments->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getValid() && n->info.getState() == 0) {
            c++;
        }
        n = n->next;
    }
    return c;
}

static bool compareAppointmentDates(const Appointment *a, const Appointment *b) {
    if (!a || !b) return a != nullptr;
    Date da = a->getDateGet();
    Date db = b->getDateGet();
    if (da.getYear() != db.getYear()) return da.getYear() < db.getYear();
    if (da.getMonth() != db.getMonth()) return da.getMonth() < db.getMonth();
    if (da.getDay() != db.getDay()) return da.getDay() < db.getDay();
    if (da.getHour() != db.getHour()) return da.getHour() < db.getHour();
    return da.getMinute() < db.getMinute();
}

void AppointmentController::sortAppointmentsByDate(QList<Appointment*> &list) {
    UIHelper::quickSort(list, compareAppointmentDates);
}

QList<Appointment*> AppointmentController::getAppointmentsForDate(const LinkList<Appointment> *appointments, const QDate &targetDate) {
    QList<Appointment*> list;
    if (!appointments || !appointments->getHeader()) return list;

    auto *n = appointments->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getValid()) {
            Date d = n->info.getDateGet();
            if (d.getYear() == targetDate.year() &&
                d.getMonth() == targetDate.month() &&
                d.getDay() == targetDate.day()) {
                list.append(&n->info);
            }
        }
        n = n->next;
    }
    sortAppointmentsByDate(list);
    return list;
}

QList<Appointment*> AppointmentController::getUpcomingAppointments(const LinkList<Appointment> *appointments, const QDate &today) {
    QList<Appointment*> list;
    if (!appointments || !appointments->getHeader()) return list;

    QDateTime now = QDateTime::currentDateTime();

    auto *n = appointments->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getValid()) {
            Date d = n->info.getDateGet();
            QDate appDay(d.getYear(), d.getMonth(), d.getDay());
            QTime appTime(d.getHour(), d.getMinute());
            QDateTime appDt(appDay, appTime);

            // Quá hạn hoặc đã khám thì không hiển thị trong lịch hẹn sắp tới
            if (appDay >= today && appDay <= today.addDays(1)) {
                if (appDt >= now && n->info.getState() == 0) {
                    list.append(&n->info);
                }
            }
        }
        n = n->next;
    }
    sortAppointmentsByDate(list);
    return list;
}

void AppointmentController::getMonthDayCounts(const LinkList<Appointment> *appointments, int year, int month, int dayCounts[32]) {
    for (int i = 0; i < 32; ++i) dayCounts[i] = 0;
    if (!appointments || !appointments->getHeader()) return;

    auto *n = appointments->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getValid()) {
            Date d = n->info.getDateGet();
            if (d.getYear() == year && d.getMonth() == month) {
                int day = d.getDay();
                if (day >= 1 && day <= 31) {
                    dayCounts[day]++;
                }
            }
        }
        n = n->next;
    }
}

std::string AppointmentController::generateNextId(const LinkList<Appointment> *appointments) {
    int maxId = 0;
    if (appointments && appointments->getHeader()) {
        auto *n = appointments->getHeader()->next;
        while (n && n->next) {
            std::string id = n->info.getId();
            if (id.rfind("A_", 0) == 0) {
                try {
                    int val = std::stoi(id.substr(2));
                    if (val > maxId) maxId = val;
                } catch (...) {}
            }
            n = n->next;
        }
    }
    char buf[32];
    std::snprintf(buf, sizeof(buf), "A_%03d", maxId + 1);
    return std::string(buf);
}

QDate AppointmentController::getLunarNewYear(int year) {
    switch (year) {
        case 2020: return QDate(2020, 1, 25);
        case 2021: return QDate(2021, 2, 12);
        case 2022: return QDate(2022, 2, 1);
        case 2023: return QDate(2023, 1, 22);
        case 2024: return QDate(2024, 2, 10);
        case 2025: return QDate(2025, 1, 29);
        case 2026: return QDate(2026, 2, 17);
        case 2027: return QDate(2027, 2, 6);
        case 2028: return QDate(2028, 1, 26);
        case 2029: return QDate(2029, 2, 13);
        case 2030: return QDate(2030, 2, 3);
        case 2031: return QDate(2031, 1, 23);
        case 2032: return QDate(2032, 2, 11);
        case 2033: return QDate(2033, 1, 31);
        case 2034: return QDate(2034, 2, 19);
        case 2035: return QDate(2035, 2, 8);
        case 2036: return QDate(2036, 1, 28);
        case 2037: return QDate(2037, 2, 15);
        case 2038: return QDate(2038, 2, 4);
        case 2039: return QDate(2039, 1, 24);
        case 2040: return QDate(2040, 2, 12);
        case 2041: return QDate(2041, 2, 1);
        case 2042: return QDate(2042, 1, 22);
        case 2043: return QDate(2043, 2, 10);
        case 2044: return QDate(2044, 1, 30);
        case 2045: return QDate(2045, 2, 17);
        default: break;
    }
    return QDate(year, 2, 1);
}

bool AppointmentController::isTetHoliday(const QDate &date, int holidayDays) {
    if (holidayDays <= 0) holidayDays = 5;
    for (int y = date.year() - 1; y <= date.year() + 1; ++y) {
        QDate mung1 = getLunarNewYear(y);
        if (!mung1.isValid()) continue;
        QDate start = mung1.addDays(-1);
        QDate end = mung1.addDays(-1 + holidayDays - 1);
        if (date >= start && date <= end) {
            return true;
        }
    }
    return false;
}

static std::string resolveHolidayPath(const std::string &path) {
    std::ifstream f(path);
    if (f.is_open()) return path;
    std::string alt = "../" + path;
    std::ifstream f2(alt);
    if (f2.is_open()) return alt;
    return path;
}

static QString findHolidayNote(const QDate &date, const std::string &configPath) {
    std::ifstream file(resolveHolidayPath(configPath));
    if (!file.is_open()) return QString();
    std::string line;
    while (std::getline(file, line)) {
        libPBL2::trim(line);
        if (line.empty()) continue;
        auto separator = line.find('-');
        std::string value = separator == std::string::npos ? line : line.substr(0, separator);
        libPBL2::trim(value);
        std::string rest = separator == std::string::npos ? "" : line.substr(separator + 1);
        libPBL2::trim(rest);

        std::string lowerVal = value;
        for (char &c : lowerVal) c = std::tolower(static_cast<unsigned char>(c));
        if (lowerVal.rfind("tet", 0) == 0) {
            int days = 5;
            if (!rest.empty()) {
                std::stringstream ss(rest);
                int parsedDays = 0;
                if (ss >> parsedDays && parsedDays > 0) {
                    days = parsedDays;
                }
            }
            if (AppointmentController::isTetHoliday(date, days)) {
                return QStringLiteral("Tết Nguyên Đán (Nghỉ %1 ngày)").arg(days);
            }
            continue;
        }

        if (value.find('/') != std::string::npos) {
            int day = 0, month = 0;
            char slash = 0;
            std::stringstream parsed(value);
            if (parsed >> day >> slash >> month && slash == '/' && day == date.day() && month == date.month()) {
                if (!rest.empty()) {
                    return QString::fromUtf8(rest.c_str());
                }
                if (day == 1 && month == 1) return QStringLiteral("Tết Dương Lịch");
                if (day == 10 && month == 3) return QStringLiteral("Giỗ Tổ Hùng Vương");
                if (day == 30 && month == 4) return QStringLiteral("Ngày Giải phóng miền Nam");
                if (day == 1 && month == 5) return QStringLiteral("Ngày Quốc tế Lao động");
                if (day == 2 && month == 9) return QStringLiteral("Quốc khánh Việt Nam");
                if (day == 1 && month == 9) return QStringLiteral("Nghỉ lễ Quốc khánh");
                return QStringLiteral("Nghỉ lễ");
            }
        }
    }
    return QString();
}

bool AppointmentController::isHoliday(const QDate &date, const std::string &configPath) {
    return !findHolidayNote(date, configPath).isEmpty();
}

QString AppointmentController::getHolidayNote(const QDate &date, const std::string &configPath) {
    QString note = findHolidayNote(date, configPath);
    return note.isEmpty() ? QStringLiteral("Nghỉ lễ") : note;
}

QList<HolidayEntry> AppointmentController::loadHolidays(const std::string &configPath) {
    QList<HolidayEntry> list;
    std::ifstream file(resolveHolidayPath(configPath));
    if (!file.is_open()) return list;
    std::string line;
    while (std::getline(file, line)) {
        libPBL2::trim(line);
        if (line.empty()) continue;
        size_t split = line.find('-');
        std::string date = split == std::string::npos ? line : line.substr(0, split);
        std::string note = split == std::string::npos ? "Nghỉ lễ" : line.substr(split + 1);
        libPBL2::trim(date);
        libPBL2::trim(note);
        list.append({QString::fromUtf8(date.c_str()), QString::fromUtf8(note.c_str())});
    }
    return list;
}

bool AppointmentController::saveHolidays(const std::string &configPath, const QList<HolidayEntry> &list) {
    std::ofstream file(resolveHolidayPath(configPath));
    if (!file.is_open()) return false;
    for (const auto &item : list) {
        file << item.date.toUtf8().toStdString() << " - " << item.note.toUtf8().toStdString() << "\n";
    }
    return true;
}
