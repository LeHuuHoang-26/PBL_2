#ifndef MANAGER_H
#define MANAGER_H

#include "human.h"
#include <fstream>
#include <string>
using namespace std;

class Manager: public Human {
	public:
		enum Role {
			Receptionist = 1,
			specialReceptionist,
			adminstrator
		};
	private:
		int role;
		bool state;
	public:
		static int count;
		
		Manager(string path = "Data/Manager.txt");
		Manager(string id, string fullName, int phone, bool gender, int age, int role, bool state);

};

#endif