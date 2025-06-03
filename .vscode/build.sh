#!/bin/bash
g++ compress.cpp -o compress -
g++ decompress.cpp -o decompress 
g++ encrypt.cpp -o encrypt -lcryptopp 
g++ decrypt.cpp -o decrypt -lcryptopp 
g++ hash.cpp -o hash -lcryptopp 
g++ metadata.cpp -o metadata -lcryptopp 
g++ time_lock.cpp -o time_lock 
g++ main.cpp -o main 
chmod +x compress decompress encrypt decrypt metadata time_lock main