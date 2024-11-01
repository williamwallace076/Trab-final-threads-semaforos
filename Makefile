# Nome do compilador
CXX = g++

# Nome do arquivo executável
TARGET = caixa_supermercado

# Opções de compilação
CXXFLAGS = -std=c++17 -pthread

# Arquivo fonte
SRCS = main.cpp

# Regra para compilar o programa
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

# Regra para limpar arquivos gerados
clean:
	rm -f $(TARGET)

# Regra padrão
.PHONY: clean
