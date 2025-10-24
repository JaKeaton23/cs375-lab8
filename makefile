CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

all: simulator

simulator: simulator.cpp
	$(CXX) $(CXXFLAGS) simulator.cpp -o simulator

clean:
	rm -f simulator

