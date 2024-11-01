# Nome do executável
TARGET = supermercado

# Compilador
CXX = g++

# Flags de compilação
CXXFLAGS = -std=c++11 -Wall -pthread

SRCS = main.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

# Regra para executar o programa
run: $(TARGET)
	./$(TARGET)

