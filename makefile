CXX = g++
CFLAGS = -Wall -g
TARGET = hw1
DEPENDENCIES = argument_parser.hpp process.hpp file.hpp

all: hw1

$(TARGET): $(TARGET).cpp $(DEPENDENCIES)
	$(CXX) -o $@ $(CFLAGS) $< 

clean:
	rm hw1
