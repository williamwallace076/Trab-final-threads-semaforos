# Definições
CXX = g++
CXXFLAGS = -std=c++20 -pthread


TARGET = supermercado


SRCS = main.cpp


all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)


clean:
	rm -f $(TARGET)
