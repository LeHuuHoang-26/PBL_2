#ifndef PATIENT_H
#define PATIENT_H

#include "human.h"
#include <string>
#include <fstream>
using namespace std;

class Patient: public Human {
	private:
		string CCCD;
	public:
		static int count;

		Patient(string id = "", string fullName = "", int phone = 0, bool gender = 0, int age = 0, string CCCD = "");
		Patient(string path);
		
};

#endif