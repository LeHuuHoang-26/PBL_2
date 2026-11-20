#include "Date.h"
#include <iomanip>
#include <sstream>
#include <ctime>

Date::Date(int day, int month, int year, int hour, int minute)
    : day(day), month(month), year(year), hour(hour), minute(minute) {}

Date::Date(const Date &D)
    : day(D.day), month(D.month), year(D.year), hour(D.hour), minute(D.minute) {}

Date::~Date() {}

int Date::getDay() const { return day; }
int Date::getMonth() const { return month; }
int Date::getYear() const { return year; }
int Date::getHour() const { return hour; }
int Date::getMinute() const { return minute; }

void Date::setDay(int d) { day = d; }
void Date::setMonth(int m) { month = m; }
void Date::setYear(int y) { year = y; }
void Date::setHour(int h) { hour = h; }
void Date::setMinute(int min) { minute = min; }

bool Date::isDefault() const {
    return (day == 0 && month == 0 && year == 0);
}

namespace {
bool isValidCalendarDateTime(int d, int m, int y, int hh, int mm) {
    if (y < 1 || y > 9999 || m < 1 || m > 12 || d < 1) return false;
    if (hh < 0 || hh > 23 || mm < 0 || mm > 59) return false;
    static const int kDaysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maxDay = kDaysInMonth[m];
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))) {
        maxDay = 29;
    }
    return d <= maxDay;
}
}

std::string Date::toString() const {
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << year << "-"
        << std::setw(2) << month << "-"
        << std::setw(2) << day << " "
        << std::setw(2) << hour << ":"
        << std::setw(2) << minute;
    return oss.str();
}

Date Date::loadFromString(const std::string &str) {
    if (str.empty()) return Date(0, 0, 0, 0, 0);
    int d = 0, m = 0, y = 0, hh = 0, mm = 0;
    char sep1 = '\0', sep2 = '\0', timeSep = '\0';
    std::istringstream iss(str);
    if (str.find('-') != std::string::npos) {
        if (iss >> y >> sep1 >> m >> sep2 >> d && sep1 == '-' && sep2 == '-') {
            if (iss >> hh >> timeSep >> mm && timeSep == ':') {
                if (isValidCalendarDateTime(d, m, y, hh, mm)) {
                    return Date(d, m, y, hh, mm);
                }
                return Date(0, 0, 0, 0, 0);
            }
            if (isValidCalendarDateTime(d, m, y, 0, 0)) {
                return Date(d, m, y, 0, 0);
            }
        }
    } else if (str.find('/') != std::string::npos) {
        if (iss >> d >> sep1 >> m >> sep2 >> y && sep1 == '/' && sep2 == '/') {
            if (iss >> hh >> timeSep >> mm && timeSep == ':') {
                if (isValidCalendarDateTime(d, m, y, hh, mm)) {
                    return Date(d, m, y, hh, mm);
                }
                return Date(0, 0, 0, 0, 0);
            }
            if (isValidCalendarDateTime(d, m, y, 0, 0)) {
                return Date(d, m, y, 0, 0);
            }
        }
    }
    return Date(0, 0, 0, 0, 0);
}

Date Date::now() {
    time_t t = time(nullptr);
    tm *curr = localtime(&t);
    if (!curr) return Date(0, 0, 0, 0, 0);
    return Date(curr->tm_mday, curr->tm_mon + 1, curr->tm_year + 1900, curr->tm_hour, curr->tm_min);
}
