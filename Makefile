all:Mengxing-v1.o

Mengxing-v1.o: Mengxing-v1.cpp
	g++ -mrtm -std=c++11 -pthread $< -o $@
