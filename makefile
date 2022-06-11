CXX = g++
CFLAGS = -Wall -g -lcapstone
TARGET = hw4
DEPENDENCIES = command.hpp parser.hpp breakpoint.hpp utils.hpp

all: hw4

$(TARGET): $(TARGET).cpp $(DEPENDENCIES)
	$(CXX) -c $(CFLAGS) $(TARGET).cpp -o $(TARGET).o
	$(CXX) $(TARGET).o $(CFLAGS) -o $(TARGET)

clean:
	rm -rf hw4 *.o core .DS_Store
