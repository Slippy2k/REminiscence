
CFLAGS += -g

all: convert_amiga_png dump decode_spl

convert_amiga_png: decode_cmp.o decode_lev.o decode_spc.o decode_spm.o decode_spr.o unpack.o util.o
	gcc -o $@ $^ -lz -lpng

dump: dump.o
	gcc $^ -o $@

decode_spl: spldecoder.o
	gcc $^ -o $@

clean:
	rm -f *.o convert_amiga_png dump decode_spl

