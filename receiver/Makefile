# Receiver Makefile
CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -I./include -I../shared/include
LDFLAGS = -L/usr/local/lib -lcryptopp -lcurl -lz -lpthread

# Targets
TARGETS = keygen decryptor

# Source files
KEYGEN_SRC = keygen.cpp ../shared/rsa_utils.cpp
KEYGEN_OBJ = $(KEYGEN_SRC:.cpp=.o)

DECRYPTOR_SRC = decryptor.cpp utils.cpp ../shared/huffman.cpp ../shared/aes_cbc.cpp ../shared/rsa_utils.cpp ../shared/hash_utils.cpp
DECRYPTOR_OBJ = $(DECRYPTOR_SRC:.cpp=.o)

# Default target
all: $(TARGETS)

keygen: $(KEYGEN_OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

decryptor: $(DECRYPTOR_OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(KEYGEN_OBJ) $(DECRYPTOR_OBJ) $(TARGETS)

# Install dependencies (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install -y libcrypto++-dev libcurl4-openssl-dev zlib1g-dev

# Run tests
test: all
	./keygen --test
	./decryptor --test

.PHONY: all clean install-deps test