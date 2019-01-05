
all: convert_3do extract_opera_iso

convert_3do: cinepak.o tga.o main.o unpack.o
	$(CXX) -o $@ $^

extract_opera_iso: extract_opera_iso.cpp
	$(CXX) -o $@ $^

clean:
	rm -f *.o
