QT += core gui widgets charts
CONFIG += c++23 console

TARGET = MngCareBook
TEMPLATE = app

# --- Cấu hình thư mục build ---
DESTDIR     = $$PWD/bin        # Exe xuất ra bin/
OBJECTS_DIR = $$PWD/release    # Object files vào release/
MOC_DIR     = $$PWD/release
RCC_DIR     = $$PWD/release
UI_DIR      = $$PWD/release

# --- Include paths ---
INCLUDEPATH += \
    $$PWD/src/Models \
    $$PWD/src/Controllers \
    $$PWD/src/Views \
    $$PWD/src/DataStructures \
    $$PWD/src/Utils

# --- Source files ---
SOURCES += \
    src/main.cpp \
    src/Models/Appointment.cpp \
    src/Models/Doctor.cpp \
    src/Models/Human.cpp \
    src/Models/Manager.cpp \
    src/Models/Patient.cpp \
    src/Models/Permission.cpp \
    src/Models/QueuePatient.cpp \
    src/Models/Symptom.cpp \
    src/Models/SymptomPatient.cpp \
    src/Utils/Date.cpp \
    src/Utils/libPBL2.cpp

# --- Header files ---
HEADERS += \
    src/Models/Appointment.h \
    src/Models/Doctor.h \
    src/Models/Human.h \
    src/Models/Manager.h \
    src/Models/Patient.h \
    src/Models/Permission.h \
    src/Models/QueuePatient.h \
    src/Models/Symptom.h \
    src/Models/SymptomPatient.h \
    src/DataStructures/LinkList.h \
    src/DataStructures/Queue.h \
    src/Utils/Date.h \
    src/Utils/libPBL2.h
