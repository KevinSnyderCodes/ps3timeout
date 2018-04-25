CC=g++

build: main.cpp
	$(CC) -std=c++11 main.cpp -ludev
