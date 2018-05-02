CC=g++

build: main.cpp
	$(CC) -std=c++11 -o ps3timeout -c main.cpp -ludev -pthread
