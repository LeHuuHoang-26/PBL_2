#ifndef SYMPTOM_H
#define SYMPTOM_H

#include <string>
#include "LinkList.h"

class Symptom {
private:
    std::string S_id;
    std::string sName;
    std::string Sp_id;
    int sScore;

public:
    Symptom();
    Symptom(std::string S_id, std::string sName, std::string Sp_id, int sScore);
    Symptom(const Symptom &S);
    Symptom &operator=(const Symptom &S) = default;
    Symptom(Symptom &&S) noexcept = default;
    Symptom &operator=(Symptom &&S) noexcept = default;
    ~Symptom() = default;

    void setDefault();
    bool isDefault() const;

    std::string getS_id() const;
    std::string getSName() const;
    std::string getSp_id() const;
    int getSScore() const;

    std::string toString() const;
    static Symptom loadFromString(const std::string &str);
    static LinkList<Symptom>* loadFromFile(const std::string &path);
};

#endif
