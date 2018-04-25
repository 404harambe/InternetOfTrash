SRC = src
OUT = out
ARDUINO = ../arduino

CXX ?= g++
CXXFLAGS += -std=c++11 -Wall -pedantic -fPIC
INCLUDES = -I$(ARDUINO) -I./wiringPi/wiringPi $(shell python-config --includes)
LDFLAGS += -fPIC -shared -L$(OUT) -lwiringPi $(shell python-config --ldflags)

OBJECTS = $(OUT)/NetworkProtocol_wrap.o \
          $(OUT)/arduino/NetworkProtocol.o \
          $(OUT)/arduino/RCSwitch.o \
          $(OUT)/arduino/crc16.o

all: default

# Compile WiringPi and copy the library to the out
wiringPi:
	$(MAKE) -C ./wiringPi/wiringPi
	@mkdir -p $(OUT)
	cp wiringPi/wiringPi/libwiringPi.so.* $(OUT)/libwiringPi.so

# Arduino libs
$(OUT)/arduino/%.o: $(ARDUINO)/%.cpp
	@mkdir -p $(OUT)/arduino
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

# Swig Python wrapper
$(OUT)/%_wrap.o: $(SRC)/%.swig
	@mkdir -p $(OUT)
	swig -python -c++ -o $(<:.swig=_wrap.cpp) $(INCLUDES) $<
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $(<:.swig=_wrap.cpp)
	cp $(<:.swig=.py) $(OUT)

# Final library
default: wiringPi $(OBJECTS)
	@mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(OUT)/_NetworkProtocol.so $(OBJECTS) $(LDFLAGS)

clean:
	rm -rf $(OUT)

.PHONY: all default wiringPi clean
