#ifndef SYMTOMP_H
#define SYMPTOM_H

#include <string>
using namespace std;

class Symptom {
    private:
        string id;
        string name;
        string Sp_id;
        int score;
    public:
        static int countSymptom = 0;

        Symptom(string path = "Data/Symptom/Symptom.txt");
};

#endif