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