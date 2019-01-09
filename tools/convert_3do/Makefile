
all: convert_3do extract_opera_iso

convert_3do: bitmap.o cinepak.o decode_3docel.o decode_3dostr.o fileio.o main.o tga.o unpack.o
	$(CXX) -o $@ $^

extract_opera_iso: extract_opera_iso.o
	$(CXX) -o $@ $^

clean:
	rm -f *.o
