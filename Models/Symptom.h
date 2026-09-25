#ifndef SYMPTOM_H
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
        static int count;

        Symptom(string path = "Data/Symptom/Symptom.txt");
};

#endif