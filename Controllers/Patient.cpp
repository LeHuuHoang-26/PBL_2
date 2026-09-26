#include "Patient.h"
#include <string>
#include <fstream>
#include "libPBL2.h"

using namespace std;

int Patient::count = 0;

Patient::Patient(string id, string fullName, int phone, bool gender, int age, string CCCD)
	:Human(id, fullName, phone, gender, age), CCCD(CCCD) {}

Patient::Patient(string path) {
	ifstream f(path);

	string tmp;
	for (int i = 0; i < Patient::count + 2; i++)
		getline(f, tmp, '\n');

	getline(f, id, '|');
	libPBL2::trim(id);

	getline(f, fullName, '|');
	libPBL2::trim(fullName);

	getline(f, tmp, '|');
	phone = stod(tmp);

	getline(f, tmp, '|');
	gender = stod(tmp);
	
	getline(f, tmp, '|');
	age = stod(tmp);
	
	getline(f, CCCD, '\n');
	libPBL2::trim(CCCD);
}