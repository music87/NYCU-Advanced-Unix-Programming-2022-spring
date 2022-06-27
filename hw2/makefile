CXX = g++
PROGS = logger logger.so #sample

all: $(PROGS)

logger: hw2.cpp
	$(CXX) -o $@ -Wall -g $<

logger.so: inject_glibc.cpp utils.hpp 
	$(CXX) -o $@ -shared -fPIC -g $< -ldl

%: %.cpp
	$(CXX) -o $@ -Wall -g $<

clean:
	rm -f *~ $(PROGS) core *.txt sample
