
CPPFLAGS += -Wall -Wpedantic

all: convert_data map2bmp

convert_data: file.o main.o unpack.o
	$(CXX) -o $@ $^ -lmodplug

map2bmp: map2bmp.o util.o
	$(CC) map2bmp.o util.o -o $@

clean:
	rm -f *.o convert_data map2bmp

