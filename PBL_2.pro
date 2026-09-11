QT += core gui widgets charts
CONFIG += c++17

TARGET = MngCareBook
TEMPLATE = app

# --- Cấu hình thư mục build ---
DESTDIR = $$PWD
OBJECTS_DIR = release
MOC_DIR = release
RCC_DIR = release
UI_DIR = release

SOURCES += \
    main.cpp \
    Controllers/AppointmentController.cpp \
    Controllers/AuthController.cpp \
    Controllers/DoctorController.cpp \
    Data_structures/BinarySearchTree.cpp \
    Data_structures/PatientQueue.cpp \
    Controllers/Appointment.cpp \
    Controllers/Schedule.cpp \
    Controllers/Users.cpp

HEADERS += \
    Models/AppointmentController.h \
    Models/AuthController.h \
    Models/DoctorController.h \
    Data_structures/BinarySearchTree.h \
    Data_structures/PatientQueue.h \
    Models/Appointment.h \
    Models/Schedule.h \
    Models/Users.h