
all: convert_mac_png

convert_mac_png: decode.o file.o main.o resource.o
	$(CXX) -o $@ $^ -lpng -lz

clean:
	rm -f *.o convert_mac_png

