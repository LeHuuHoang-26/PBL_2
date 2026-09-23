#include "libPBL2.cpp"
#include <string>
using namespace std;

void libPBL2::trim(string &str) {
    int i = 0;
    while (i < str.length() && str[i] == ' ')
        i++;

    for (int j = i; j < str.length(); j++)
        str[j - i] = str[j];

    str.resize(str.length() - i);

    i = str.length() - 1;
    while (i >= 0 && str[i] == ' ')
        i--;

    str.resize(i + 1);
}