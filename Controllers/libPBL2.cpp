#include "libPBL2.h"
#include <string>
#include <fstream>
using namespace std;

void libPBL2::trim(string &str) {
    if (str.empty()) 
        return;

    size_t i = 0;
    while (i < str.length() && str[i] == ' ')
        i++;
    if (i == str.length()) {
        str.clear();
        return;
    }

    for (size_t j = i; j < str.length(); j++)
        str[j - i] = str[j];

    str.resize(str.length() - i);


    int k = static_cast<int>(str.length()) - 1;
    while (k >= 0 && str[k] == ' ')
        k--;

    str.resize(k + 1);
}
