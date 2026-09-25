#include "Patient.h"
#include <string>
#include <fstream>

using namespace std;

int Patient::count = 0;

Patient::Patient(string path) {
	ifstream f(path);

	string tmp;
	for (int i = 0; i < Patient::count + 2; i++)
		getline(f, tmp, '\n');
}