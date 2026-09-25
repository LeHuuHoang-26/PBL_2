#ifndef SYMPTOMPATIENT_H
#define SYMPTOMPATIENT_H

#include <string>
using namespace std;

class SymptomPT {
    private:
        string S_id;
        string P_id;
    public:
        static int count;
        
        SymptomPT(string path = "Data/Symptom/SymptomPatient.txt");
        SymptomPT(string S_id = "", string P_id = "");
};

#endif