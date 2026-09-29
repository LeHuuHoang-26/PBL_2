#ifndef DOCTOR_H
#define DOCTOR_H

#include "human.h"
#include <string>
#include "../Data_structures/LinkList.h"

class Doctor : public Human {
private:
    std::string spId;        // Sp_ID
    bool state;              // D_State
    std::string roomId;      // D_RoomID (Thuộc tính bổ sung)
    double fee;              // D_Fee (Thuộc tính bổ sung)
    double rating;           // D_Rating (Thuộc tính bổ sung)

public:
    // Hàm khởi tạo đúng chuẩn format CSDL mở rộng
    Doctor(std::string _id = "", std::string _fullName = "", std::string _phone = "", 
           bool _gender = true, int _age = 0, std::string _spId = "", bool _state = true,
           std::string _roomId = "", double _fee = 0.0, double _rating = 5.0);

    virtual ~Doctor();

    void displayInfo() const override;

    // Getter
    std::string getSpId() const;
    bool getState() const;
    std::string getRoomId() const;
    double getFee() const;
    double getRating() const;

    // Setter
    void setSpId(std::string _spId);
    void setState(bool _state);
    void setRoomId(std::string _roomId);
    void setFee(double _fee);
    void setRating(double _rating);

    // Đọc danh sách bác sĩ từ file .txt
    static LinkList<Doctor> loadFromFile(const std::string& filename);
};

#endif // DOCTOR_H