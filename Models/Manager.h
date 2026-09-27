#ifndef MANAGER_H
#define MANAGER_H

#include "human.h"
#include <fstream>
#include <string>
using namespace std;

class Manager: public Human {
	public:
		enum Role {
			Empty = 0,
			Receptionist,
			specialReceptionist,
			adminstrator
		};
	private:
		string username;
		string password;
		int role;
		bool state;
	public:
		static int count;
		
		Manager(string path);
		Manager(string id = "", string fullName = "", int phone = 0, bool gender = 0, int age = 0, string username = "", string password = "", int role = 0, bool state = 0);
		bool verifyAccount(string username, string password) const;
		bool verifyInfo(const Manager *M) const;
};

#endif