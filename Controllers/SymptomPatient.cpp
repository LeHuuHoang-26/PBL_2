#include "SymptomPatient.h"
#include <string>
#include <fstream>
#include "libPBL2.h"
using namespace std;

int SymptomPT::count = 0;

SymptomPT::SymptomPT(string path) {
    ifstream f(path);

    string tmp;
    for (int i = 0; i < SymptomPT::count + 2; i++)
        getline (f, tmp, '\n');

    getline (f, S_id, '|');
    getline (f, P_id, '\n');

    libPBL2::trim(S_id);
    libPBL2::trim(P_id);

    SymptomPT::count++;
}

SymptomPT::SymptomPT(string S_id, string P_id): S_id(S_id), P_id(P_id) {
    SymptomPT::count++;
}