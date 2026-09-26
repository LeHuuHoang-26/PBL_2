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

		Patient(string id, string fullName, int phone, bool gender, int age, string CCCD);
		Patient(string path = "Data/Patient.txt");
		
};

#endif