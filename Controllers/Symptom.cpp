#include "Symptom.h"
#include "libPBL2.h"
#include <bits/stdc++.h>
#include <string>
using namespace std;


void Symptom::getData() {
    ifstream f("Data/Symptom/Symptom.txt");
    string tmp;
    for (int i = 0; i < 2 + countSymptom; i++)
        getline(f, tmp, '\n');

    getline (f, id, '|');
    getline (f, name, '|');
    getline (f, Sp_id, '|');
    getline (f, tmp , '\n');
    libPBL2::trim(id);
    libPBL2::trim(name);
    libPBL2::trim(Sp_id);
    score = stod(tmp);

    countSymptom++;
}