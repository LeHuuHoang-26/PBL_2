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

# =========================
# Project
# =========================
TARGET = PBL_2.exe

SOURCES := $(shell find . -name "*.cpp")
OBJECTS := $(SOURCES:.cpp=.o)
DEPS    := $(OBJECTS:.o=.d)

# =========================
# MOC - tự động dò header có Q_OBJECT
# =========================
MOC_HEADERS := $(shell grep -rl "Q_OBJECT" --include="*.h" .)
MOC_SOURCES := $(MOC_HEADERS:.h=_moc.cpp)
MOC_OBJECTS := $(MOC_SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS) $(MOC_OBJECTS)
	$(CXX) $(OBJECTS) $(MOC_OBJECTS) -o $@ $(LDFLAGS) $(LDLIBS)

-include $(DEPS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

%_moc.cpp: %.h
	$(MOC) $< -o $@

%_moc.o: %_moc.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(DEPS) $(MOC_SOURCES) $(MOC_OBJECTS) $(TARGET)

.PHONY: all clean