#ifndef DOCTOR_H
#define DOCTOR_H

#include "Human.h"
#include "LinkList.h"
#include <string>

class Doctor : public Human {
private:
    std::string spId;
    int state;
    std::string roomId;
    double fee;
    double rating;

public:
    Doctor(std::string _id = "", std::string _fullName = "", std::string _phone = "", bool _gender = true, int _age = 0, std::string _spId = "", int _state = 1, std::string _roomId = "", double _fee = 0.0, double _rating = 5.0);
    Doctor(const Doctor &) = default;
    Doctor &operator=(const Doctor &) = default;
    Doctor(Doctor &&) noexcept = default;
    Doctor &operator=(Doctor &&) noexcept = default;
    ~Doctor() = default;

    bool isDefault() const;
    void setDefault();

    std::string getSpId() const;
    int getState() const;
    std::string getRoomId() const;
    double getFee() const;
    double getRating() const;

    void setSpId(const std::string &_spId);
    void setState(int _state);
    void setRoomId(const std::string &_roomId);
    void setFee(double _fee);
    void setRating(double _rating);

    std::string toString() const;
    static Doctor loadFromString(const std::string &str);
    static LinkList<Doctor>* loadFromFile(const std::string &path);
    static bool saveToFile(const std::string &path, const LinkList<Doctor> *list);
};

#endif
