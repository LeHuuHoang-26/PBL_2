#include "libPBL2.h"
#include <string>
#include <fstream>
using namespace std;

void libPBL2::trim(string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    (start == string::npos) ? "" : s.substr(start, end - start + 1);
}
