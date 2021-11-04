CFLAGS=-std=c++11 -Ofast -fopenmp -march=native -fPIC
LFLAGS=-shared -d2 

globimap.so: globimap.o
	g++ $(LFLAGS) -o globimap.so globimap.o  -lgomp
globimap.o: globimap.hpp globimap.cpp
	g++ $(CFLAGS) -o globimap.o -c globimap.cpp   `pkg-config --cflags python3`

clean:
	rm -f globimap.o globimap.so 

