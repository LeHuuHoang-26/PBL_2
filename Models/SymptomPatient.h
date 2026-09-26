#ifndef SYMPTOMPATIENT_H
#define SYMPTOMPATIENT_H

#include "libPBL2.h"
#include "Date.h"
#include <string>
using namespace std;

class SymptomPT {
    private:
        string S_id;
        string P_id;
        Date date;
    public:
        static int count;
        
        SymptomPT(string path = "Data/Symptom/SymptomPatient.txt");
        SymptomPT(const string &S_id, const string &P_id, const Date &SP_date);
};

#endif