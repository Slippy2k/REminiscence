
all: convert_data map2bmp

convert_data: file.o main.o
	$(CXX) -o $@ $^ -lmodplug

map2bmp: map2bmp.o util.o
	$(CC) $(LDFLAGS) map2bmp.o util.o -o $@

clean:
	rm -f *.o convert_data map2bmp

