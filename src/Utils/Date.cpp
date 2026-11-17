#include "Date.h"
#include <iomanip>
#include <sstream>
#include <ctime>

using namespace std;

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

int Date::posofdayinmonth(int monthVar) const {
    if (monthVar == 0)
        monthVar = month;
    if (monthVar < 1 || monthVar > 12)
        return 0;
    int dayinmonth_list[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (this->isLeapYear() && monthVar == 2)
        return 29;
    return dayinmonth_list[monthVar];
}

bool Date::isLeapYear(int yearVar) const {
    if (yearVar == 0)
        yearVar = year;
    if ((yearVar % 4 == 0 && yearVar % 100 != 0) || yearVar % 400 == 0)
        return 1;
    return 0;
}

Date Date::operator - (const int &value) {
    if (value <= 0) return *this;
    Date tmp = *this;
    int remaining = value;
    while (remaining > 0) {
        if (tmp.day > remaining) {
            tmp.day -= remaining;
            remaining = 0;
        } else {
            remaining -= tmp.day;
            tmp.month--;
            if (tmp.month < 1) {
                tmp.month = 12;
                tmp.year--;
                if (tmp.year < 1) {
                    tmp.year = 1;
                    tmp.month = 1;
                    tmp.day = 1;
                    return tmp;
                }
            }
            tmp.day = tmp.posofdayinmonth(tmp.month);
        }
    }
    return tmp;
}

bool Date::isDefault() const {
    return (day == 0 && month == 0 && year == 0);
}

string Date::toString() const {
    ostringstream oss;
    oss << setfill('0')
        << setw(4) << year << "-"
        << setw(2) << month << "-"
        << setw(2) << day << " "
        << setw(2) << hour << ":"
        << setw(2) << minute;
    return oss.str();
}

Date Date::loadFromString(const string &str) {
    if (str.empty()) return Date(0, 0, 0, 0, 0);
    int d = 0, m = 0, y = 0, hh = 0, mm = 0;
    char sep1 = '\0', sep2 = '\0', timeSep = '\0';
    istringstream iss(str);
    if (str.find('-') != string::npos) {
        if (iss >> y >> sep1 >> m >> sep2 >> d && sep1 == '-' && sep2 == '-') {
            if (iss >> hh >> timeSep >> mm && timeSep == ':') {
                return Date(d, m, y, hh, mm);
            }
            return Date(d, m, y, 0, 0);
        }
    } else if (str.find('/') != string::npos) {
        if (iss >> d >> sep1 >> m >> sep2 >> y && sep1 == '/' && sep2 == '/') {
            if (iss >> hh >> timeSep >> mm && timeSep == ':') {
                return Date(d, m, y, hh, mm);
            }
            return Date(d, m, y, 0, 0);
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
