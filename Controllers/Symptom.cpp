#include "Symptom.h"
#include "libPBL2.h"
#include <fstream>
#include <string>
using namespace std;

int Symptom::count = 0;

Symptom::Symptom(string path) {
    ifstream f(path);
    string tmp;
    for (int i = 0; i < 2 + Symptom::count; i++)
        getline(f, tmp, '\n');

    getline (f, id, '|');
    getline (f, name, '|');
    getline (f, Sp_id, '|');
    getline (f, tmp , '\n');
    libPBL2::trim(id);
    libPBL2::trim(name);
    libPBL2::trim(Sp_id);
    score = stod(tmp);

    Symptom::count++;
}