#include "SymptomPatient.h"
#include "Date.h"
#include <string>
#include <fstream>
#include "libPBL2.h"
using namespace std;

int SymptomPT::count = 0;

SymptomPT::SymptomPT(string path) {
    ifstream f(path);

    string tmp;
    for (int i = 0; i < SymptomPT::count + 2; i++)
        getline(f, tmp, '\n');

    getline(f, S_id, '|');
    getline(f, P_id, '|');

    int day, month, year;
    getline(f, tmp, '.');
    day = stod(tmp);
    getline(f, tmp, '.');
    month = stod(tmp);
    getline(f, tmp, '\n');
    year = stod(tmp);

    date.insert(day, month, year);

    libPBL2::trim(S_id);
    libPBL2::trim(P_id);

    SymptomPT::count++;
}

SymptomPT::SymptomPT(const string &S_id, const string &P_id, const Date &SP_date): S_id(S_id), P_id(P_id), date(SP_date) {
    SymptomPT::count++;
}