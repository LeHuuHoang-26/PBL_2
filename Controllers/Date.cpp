#include "Date.h"
#include <iostream>
#include <iomanip>
using namespace std;

Date::Date(int day, int month, int year, int hour, int minute)
    : day(day), month(month), year(year), hour(hour), minute(minute) {}

Date::Date(const Date &D) {
    this->day = D.day;
    this->month = D.month;
    this->year = D.year;
    this->hour = D.hour;
    this->minute = D.minute;
}

Date::~Date() {}

istream &operator >> (istream &in, Date &D) {
    cout << "Date and time (dd/mm/yyyy hh:mm): ";
    char dateSeparator1, dateSeparator2, timeSeparator;
    while (in >> D.day >> dateSeparator1 >> D.month >> dateSeparator2
              >> D.year >> D.hour >> timeSeparator >> D.minute) {
        if (dateSeparator1 == '/' && dateSeparator2 == '/'
            && timeSeparator == ':' && D.year >= 1
            && D.month >= 1 && D.month <= 12
            && D.day >= 1 && D.day <= D.posofdayinmonth()
            && D.hour >= 0 && D.hour < 24
            && D.minute >= 0 && D.minute < 60)
            return in;
        cout << "Nhap lai (dd/mm/yyyy hh:mm): ";
    }

    return in;
}

ostream &operator << (ostream &out, const Date &D) {
    out << "Date and time (dd/mm/yyyy hh:mm): ";
    out << setfill('0');
    out << setw(2) << D.day << '/' 
        << setw(2) << D.month << '/' 
        << setw(4) << D.year << ' '
        << setw(2) << D.hour << ':'
        << setw(2) << D.minute;

    out << endl;
    return out;
}

int Date::posofdayinmonth(int monthVar) const {
    if (monthVar == 0)
        monthVar = month;
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

Date Date::operator ++ (int) {
    Date tmp = *this;
    int dayinmonth = this->posofdayinmonth();
    day++;
    if (day > dayinmonth) {
        day = 1;
        month++;
        if (month > 12) {
            month = 1;
            year++;
        }
    }

    return tmp;
}

Date Date::operator -- (int) {
    Date tmp = *this;

    if (day == 1 && month == 1 && year == 1){
        cout << "Can't decrease this date!\nIt's not change!\n";
        return tmp;
    }
    day--;
    if (day <= 0) {
        month--;
        if (month <= 0){
            year--;
            month = 12;
        }
        int dayinmonth = this->posofdayinmonth();
        day = dayinmonth;
    }

    return tmp;
}

Date Date::operator ++ () {
    int dayinmonth = this->posofdayinmonth();
    day++;
    if (day > dayinmonth) {
        day = 1;
        month++;
        if (month > 12) {
            month = 1;
            year++;
        }
    }

    return *this;
}

Date Date::operator -- () {
    if (day == 1 && month == 1 && year == 1){
        cout << "Can't decrease this date!\nIt's not change!\n";
        return *this;
    }
    day--;
    if (day <= 0) {
        month--;
        if (month <= 0) {
            year--;
            month = 12;
        }
        int dayinmonth = this->posofdayinmonth();
        day = dayinmonth;
    }

    return *this;
}

Date Date::operator + (const int &value) {
    Date tmp = *this;
    int valueVar = value + tmp.day - 1;
    tmp.day = 1;

    while (tmp.month > 1) {
        valueVar += tmp.posofdayinmonth(--tmp.month);
    }

    int tmpVar = tmp.isLeapYear() ? 366 : 365;
    while (valueVar >= tmpVar) {
        valueVar -= tmpVar;
        tmp.year++;

        tmpVar = tmp.isLeapYear() ? 366 : 365;
    }
    tmpVar = tmp.posofdayinmonth();
    while (valueVar >= tmpVar) {
        valueVar -= tmpVar;
        if (++tmp.month > 12) {
            tmp.month = 1;
            tmp.year++;
        }

        tmpVar = tmp.posofdayinmonth();
    
    }
    tmp.day += valueVar;

    return tmp;
}

Date Date::operator - (const int &value) {
    Date tmp = *this;

    int tmpVar = tmp.posofdayinmonth();
    int valueVar = value + (tmpVar - tmp.day);

    tmp.day = tmpVar;

    while (tmp.month < 12) {
        valueVar += tmp.posofdayinmonth(++tmp.month);
    }

    tmpVar = tmp.isLeapYear() ? 366 : 365;
    while (valueVar >= tmpVar) {
        if (tmp.year == 1) {
            tmp.day = 1;
            tmp.year = 1;
            return tmp;
        }
        tmp.year--;
        valueVar -= tmpVar;
        tmpVar = tmp.isLeapYear() ? 366 : 365;
    }

    tmpVar = tmp.posofdayinmonth();
    while (valueVar >= tmpVar) {
        tmp.month--;
        valueVar -= tmpVar;

        tmpVar = tmp.posofdayinmonth();
        tmp.day = tmpVar;
    }

    tmp.day -= valueVar;

    return tmp;
}

void Date::insert(int dd, int mm, int yyyy, int hh, int min) {
    day = dd;
    month = mm;
    year = yyyy;
    hour = hh;
    minute = min;
}

int Date::getHour() const { return hour; }
int Date::getMinute() const { return minute; }

void Date::show() const {
    cout << "Date and time (dd/mm/yyyy hh:mm): ";
    cout << setfill('0');
    cout << setw(2) << day << '/' 
        << setw(2) << month << '/' 
        << setw(4) << year << ' '
        << setw(2) << hour << ':'
        << setw(2) << minute;

    cout << endl;
}
