#ifndef PATIENT_H
#define PATIENT_H

#include "human.h"
#include <string>
#include <fstream>
using namespace std;

class Patient: public Human {
	private:
		string CCCD;
		unsigned int Age;
	public:
		static int count;
		Patient (string path = "Data/Patient.txt");
		Patient (string id = "", string LastName = "", string MiddleName = "", string FirstName = "", string PhoneNumbers = "", string Gender = "", string cccd = "", string Age = "");				
};

#endif