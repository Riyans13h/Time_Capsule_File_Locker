# Sender Makefile
CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -I./include -I../shared/include
LDFLAGS = -L/usr/local/lib -lcryptopp -lcurl -lz -lpthread

# Source files
SRC = encryptor.cpp utils.cpp ../shared/huffman.cpp ../shared/aes_cbc.cpp ../shared/rsa_utils.cpp ../shared/hash_utils.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = encryptor

# Default target
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJ) $(TARGET)

# Install dependencies (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install -y libcrypto++-dev libcurl4-openssl-dev zlib1g-dev

# Run tests
test: $(TARGET)
	./$(TARGET) --test

.PHONY: all clean install-deps test