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
    Controllers/Appointment.cpp \
    Controllers/AuthController.cpp \
    Controllers/Doctor.cpp \
    Data_structures/LinkingList.cpp \
    Data_structures/PatientPriorityQueue.cpp \
    Controllers/Patient.cpp \
    Controllers/Symptom.cpp

HEADERS += \
    Models/Appointment.h \
    Models/AuthController.h \
    Models/Doctor.h \
    Data_structures/LinkingList.h \
    Data_structures/PatientPriorityQueue.h \
    Models/Patient.h \
    Models/Symptom.h