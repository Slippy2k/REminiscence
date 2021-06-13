
all: convert_mac_png

convert_mac_png: cinepak.o decode.o file.o main.o resource.o scale2x.o scale3x.o scalebit.o
	$(CXX) -o $@ $^ -lpng -lz

clean:
	rm -f *.o convert_mac_png

