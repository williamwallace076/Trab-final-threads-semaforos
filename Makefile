CXX = g++
CXXFLAGS = -pthread

all: supermercado

supermercado: supermercado.cpp
	$(CXX) -o supermercado supermercado.cpp $(CXXFLAGS)

clean:
	rm -f supermercado
