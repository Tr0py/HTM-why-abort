all:Mengxing-v1.o Mengxing-v2.o Jim-touch.o Jim-touch-v2.o

Mengxing-v1.o: Mengxing-v1.cpp
	g++ -mrtm -O3 -std=c++11 -pthread $< -o $@
Mengxing-v2.o: Mengxing-v2.cpp
	g++ -mrtm -O3 -std=c++11 -pthread $< -o $@
Jim-touch.o: Jim-touch.cpp
	g++ -mrtm -O3 -std=c++11 -pthread $< -o $@
Jim-touch-v2.o: Jim-touch-v2.cpp
	g++ -mrtm -O3 -std=c++11 -pthread $< -o $@

clean:
	rm Mengxing-v1.o Mengxing-v2.o Jim-touch.o Jim-touch-v2.o

