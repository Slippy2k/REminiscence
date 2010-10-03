
all: decode_lev

decode_lev: decode_lev.o unpack.o
	gcc $^ -o $@ -lz -lpng

clean:
	rm -f *.o decode_lev

