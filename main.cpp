# =========================
# Compiler & Qt tools
# =========================
CXX = g++
MOC = C:/msys64/ucrt64/bin/moc.exe

QT_INC = C:/msys64/ucrt64/include
QT_LIB = C:/msys64/ucrt64/lib

# =========================
# Compiler flags
# =========================
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 \
           -I. \
           -I$(QT_INC) \
           -I$(QT_INC)/QtCore \
           -I$(QT_INC)/QtGui \
           -I$(QT_INC)/QtWidgets

LDFLAGS = -L$(QT_LIB)
LDLIBS  = -lQt6Widgets -lQt6Gui -lQt6Core -mwindows
# -mwindows: ẩn console đen khi chạy app GUI

# =========================
# Project
# =========================
TARGET = PBL_2.exe

# Tự động gom tất cả .cpp trong project
SOURCES := $(shell find . -name "*.cpp")
OBJECTS := $(SOURCES:.cpp=.o)
DEPS    := $(OBJECTS:.o=.d)
# .d chứa danh sách header mà từng .cpp phụ thuộc
# -> sửa .h cũng khiến file liên quan được build lại, không chỉ sửa .cpp

# =========================
# MOC (Meta-Object Compiler)
# =========================
# Liệt kê các header có dùng Q_OBJECT (signal/slot) tại đây
MOC_HEADERS := Controllers/AppointmentController.h \
               Controllers/AuthController.h \
               Controllers/DoctorController.h

MOC_SOURCES := $(MOC_HEADERS:.h=_moc.cpp)
MOC_OBJECTS := $(MOC_SOURCES:.cpp=.o)

# =========================
# Default target
# =========================
all: $(TARGET)

# =========================
# Link
# =========================
$(TARGET): $(OBJECTS) $(MOC_OBJECTS)
	$(CXX) $(OBJECTS) $(MOC_OBJECTS) -o $@ $(LDFLAGS) $(LDLIBS)

# Nạp file phụ thuộc header (bỏ qua nếu chưa tồn tại lần build đầu)
-include $(DEPS)

# =========================
# Compile rules
# =========================
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

%_moc.cpp: %.h
	$(MOC) $< -o $@

%_moc.o: %_moc.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# =========================
# Clean
# =========================
clean:
	rm -f $(OBJECTS) $(DEPS) $(MOC_SOURCES) $(MOC_OBJECTS) $(TARGET)

.PHONY: all clean