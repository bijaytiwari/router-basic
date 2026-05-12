CXX=g++
CXXFLAGS=-std=c++17 -Wall -Wextra -O2

SRC=src/main.cpp src/router.cpp
TARGET=router-basic

all:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
