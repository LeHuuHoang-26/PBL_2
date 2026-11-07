#ifndef SYMPTOMPATIENTDETAIL_H
#define SYMPTOMPATIENTDETAIL_H

#include <string>
#include <fstream>
#include "LinkList.h"

class SymptomPatientDetail {
private:
    std::string SP_id;
    std::string S_id;

public:
    SymptomPatientDetail();
    SymptomPatientDetail(std::string SP_id, std::string S_id);
    SymptomPatientDetail(const SymptomPatientDetail &other);
    SymptomPatientDetail &operator=(const SymptomPatientDetail &other) = default;
    ~SymptomPatientDetail() = default;

    void setDefault();
    bool isDefault() const;

    std::string getSP_id() const;
    std::string getS_id() const;

    void setSP_id(const std::string &val);
    void setS_id(const std::string &val);

    std::string toString() const;
    static SymptomPatientDetail loadFromString(const std::string &str);
    static LinkList<SymptomPatientDetail>* loadFromFile(const std::string &path);
    static bool saveToFile(const std::string &path, const LinkList<SymptomPatientDetail> *list);
};

#endif // SYMPTOMPATIENTDETAIL_H
