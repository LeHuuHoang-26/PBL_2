#include "Manager.h"
#include "human.h"
#include "libPBL2.h"
#include <string>
#include <fstream>
using namespace std;

int Manager::count = 0;

Manager::Manager(string path) {
	if (path.empty()) {
		username = "";
		password = "";
		role = 0;
		state = 0;

		return;
	}

	ifstream f(path);
	if (!f.is_open()) {
		return;
	}

	string tmp;
	for (int i = 0; i < Manager::count + 2; i++)
		getline(f, tmp, '\n');

	getline(f, id, '|');
	libPBL2::trim(id);

	getline(f, fullName, '|');
	libPBL2::trim(fullName);

	getline(f, tmp, '|');
	phone = stoi(tmp);

	getline(f, tmp, '|');
	gender = stoi(tmp) != 0;

	getline(f, tmp, '|');
	age = stoi(tmp);

	getline(f, username, '|');
	libPBL2::trim(username);

	getline(f, password, '|');
	libPBL2::trim(password);

	getline(f, tmp, '|');
	role = stoi(tmp);

	getline(f, tmp, '\n');
	state = stoi(tmp) != 0;

	Manager::count++;
}

Manager::Manager(string id, string fullName, int phone, bool gender, int age, string username, string password, int role, bool state)
	: Human(id, fullName, phone, gender, age), username(username), password(password), role(role), state(state) {
	Manager::count++;
}

bool Manager::verifyAccount(string username, string password) const {
	return this->username == username && this->password == password;
}

bool Manager::verifyInfo(const Manager *M) const {
	const Human *H1 = this;
	const Human *H2 = M;

	return *H1 == *H2;
}
