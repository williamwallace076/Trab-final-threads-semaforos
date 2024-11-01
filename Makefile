# Definições
CXX = g++
CXXFLAGS = -std=c++20 -pthread

TARGET = supermercado

SRCS = supermercado.cpp


all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)


clean:
	rm -f $(TARGET)
