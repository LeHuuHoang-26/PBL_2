QT += core gui widgets charts
CONFIG += c++23

TARGET = MngCareBook
TEMPLATE = app

# --- Cấu hình thư mục build ---
DESTDIR = $$PWD
OBJECTS_DIR = release
MOC_DIR = release
RCC_DIR = release
UI_DIR = release

INCLUDEPATH += \
    $$PWD/Models \
    $$PWD/Controllers \
    $$PWD/Data_structures

SOURCES += \
    main.cpp \
    Controllers/Appointment.cpp \
    Controllers/AuthController.cpp \
    Controllers/Doctor.cpp \
    Controllers/Patient.cpp \
    Controllers/Symptom.cpp \
    Controllers/human.cpp \
    Controllers/libPBL2.cpp \
    Controllers/SymptomPatient.cpp
    

HEADERS += \
    Models/Appointment.h \
    Models/AuthController.h \
    Models/Doctor.h \
    Models/Patient.h \
    Models/Symptom.h \
    Models/human.h \
    Models/libPBL2.h \
    Models/SymptomPatient.h \
    Data_structures/LinkList.h \
    Data_structures/Queue.h
