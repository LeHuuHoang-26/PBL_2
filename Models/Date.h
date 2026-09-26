#ifndef DATE_H
#define DATE_H

#include <iostream>
using namespace std;

class Date {
    private:
        int day;
        int month;
        int year;
    public:
        Date(int day = 0, int month = 0, int year = 0);
        Date(const Date &D);
        ~Date();

        void insert(int dd, int mm, int yyyy);
        void show() const;
        friend istream &operator >> (istream &in, Date &D);
        friend ostream &operator << (ostream &out, const Date &D);
        Date operator ++ (int);
        Date operator -- (int);
        Date operator ++ ();
        Date operator -- ();
        Date operator + (const int &value);
        Date operator - (const int &value);
        int posofdayinmonth(int = 0) const;
        bool isLeapYear(int = 0) const;
};

#endif