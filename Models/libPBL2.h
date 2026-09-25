#ifndef LIBPBL2_H
#define LIBPBL2_H
#include <string>
using namespace std;

class libPBL2 {
    public:
        static void trim(string &);
        static bool isFull(string path = "", int countLine = 0);
        static int countLine(string path);
};

#endif