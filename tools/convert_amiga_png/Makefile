
CFLAGS += -g

all: convert_amiga_png

convert_amiga_png: decode_cmp.o decode_lev.o unpack.o
	gcc -o $@ $^ -lz -lpng

clean:
	rm -f *.o convert_amiga_png

