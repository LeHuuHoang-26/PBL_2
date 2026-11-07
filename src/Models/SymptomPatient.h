#ifndef SYMPTOMPATIENT_H
#define SYMPTOMPATIENT_H

#include <string>
#include <fstream>
#include "Date.h"
#include "LinkList.h"

class SymptomPatient {
private:
    std::string SP_id;
    std::string P_id;
    std::string Sp_id;
    Date date;

public:
    SymptomPatient();
    SymptomPatient(std::string SP_id, std::string P_id, std::string Sp_id = "", Date date = Date());
    SymptomPatient(const SymptomPatient &SP);
    SymptomPatient &operator=(const SymptomPatient &SP) = default;
    ~SymptomPatient() = default;

    void setDefault();
    bool isDefault() const;

    std::string getSP_id() const;
    std::string getP_id() const;
    std::string getSp_id() const;
    Date getDate() const;

    void setSP_id(const std::string &SP_id);
    void setP_id(const std::string &P_id);
    void setSp_id(const std::string &Sp_id);
    void setDate(const Date &date);

    std::string toString() const;
    static SymptomPatient loadFromString(const std::string &str);
    static LinkList<SymptomPatient>* loadFromFile(const std::string &path);
    static bool saveToFile(const std::string &path, const LinkList<SymptomPatient> *list);
    static std::string generateNextId(const LinkList<SymptomPatient> *list);
};

#endif // SYMPTOMPATIENT_H
