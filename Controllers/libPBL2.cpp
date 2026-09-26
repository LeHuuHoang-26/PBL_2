#include "libPBL2.h"
#include <string>
#include <fstream>
using namespace std;

void libPBL2::trim(string &str) {
    size_t i = 0;
    while (i < str.length() && str[i] == ' ')
        i++;

    for (size_t j = i; j < str.length(); j++)
        str[j - i] = str[j];

    str.resize(str.length() - i);

    i = str.length() - 1;
    while (str[i] == ' ')
        i--;

    str.resize(i + 1);
}

bool libPBL2::isFull(string path, int countLine) {
    ifstream f(path);

    string tmp;
    for (int i = 0; i < countLine + 2; i++)
        getline(f, tmp, '\n');

    char c;
    while (f.get(c)) {
        if (c != '\n' && c != ' ' && c != '\t')
            return true;
    }

    return false;   
}

int libPBL2::countLine(string path) {
    ifstream f(path);

    string tmp;
    int count = -2;
    
    while (getline(f, tmp, '\n'))
        count++;
    return count;
}   