QT += core gui widgets charts
CONFIG += c++17
 
TARGET = PBL_2
TEMPLATE = app
 
SOURCES += \
    main.cpp \
    Controllers/AppointmentController.cpp \
    Controllers/AuthController.cpp \
    Controllers/DoctorController.cpp \
    Data_structures/BinarySearchTree.cpp \
    Data_structures/PatientQueue.cpp \
    Models/Appointment.cpp \
    Models/Schedule.cpp \
    Models/Users.cpp
 
HEADERS += \
    Controllers/AppointmentController.h \
    Controllers/AuthController.h \
    Controllers/DoctorController.h \
    Data_structures/BinarySearchTree.h \
    Data_structures/PatientQueue.h \
    Models/Appointment.h \
    Models/Schedule.h \
    Models/Users.h
 
# Nếu 1 class nào đó KHÔNG có .cpp riêng (viết hết trong .h, kiểu header-only,
# thường gặp với template hoặc class ngắn), xoá dòng .cpp tương ứng ở SOURCES —
# để nguyên sẽ báo lỗi "no such file" vì qmake không tìm thấy file đó.