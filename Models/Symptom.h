#ifndef SYMPTOM_H
#define SYMPTOM_H

#include <bits/stdc++.h>

// Symptom kế thừa 
class Symptom : public  {
    private:
        string id;
        string name;
        string Sp_id;
        string score;
    public:
        Symptom();
        Symptom(const Symptom &S);
        ~Symptom();
}

#endif //SYMPTOM_H
