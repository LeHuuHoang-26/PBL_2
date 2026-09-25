#include "SymptomPatient.h"
#include <string>
#include "libPBL2.h"
using namespace std;


SymptomPatient::SymptomPatient(string path) {
    ifstream f(path);

    string tmp;
    for (int i = 0; i < countSymptomPT; i++)
        getline (f, tmp, "\n");

    getline (f, S_id, '|');
    getline (f, P_id, '\n');

    libPBL2::trim(S_id);
    libPBL2::trim(P_id);

    countSymptomPT++;
}

SymptomPatient::SymptomPatient(string S_id, string P_id): S_id(S_id), P_id(P_id) {
    countSymptomPT++;
}

