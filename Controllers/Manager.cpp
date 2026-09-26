#include "Manager.h"
#include "human.h"
#include "libPBL2.h"
#include <string>
using namespace std;

int Manager::count = 0;

Manager::Manager(string path) {
	ifstream f(path);

	string tmp;
	for (int i = 0; i < Manager::count + 2; i++)
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

	getline(f, tmp, '|');
	role = stod(tmp);

	getline(f, tmp, '\n');
	state = stod(tmp);

}

Manager::Manager(string id, string fullName, int phone, bool gender, int age, int role, bool state)
	: Human(id, fullName, phone, gender, age), role(role), state(state) {}
