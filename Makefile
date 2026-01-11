# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -pthread -std=c++11

# Target executable name
TARGET = http_server

# Default target
all: $(TARGET)

$(TARGET): server.cpp
 $(CXX) $(CXXFLAGS) -o $(TARGET) server.cpp

# Clean up binaries
clean:
 rm -f $(TARGET)
