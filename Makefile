CC = g++


all: main

main: main.cpp *.hpp
	g++ --std=c++20 -Wall -Wextra -Wpedantic -Werror -lfmt main.cpp -omain

.PHONY: all