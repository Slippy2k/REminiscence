
DEFINES = -DSYS_LITTLE_ENDIAN

CC = gcc
CFLAGS:= -g -O -std=c89 -pedantic -Wuninitialized -Wno-multichar


all: map2bmp

map2bmp: map2bmp.o util.o
	$(CC) $(LDFLAGS) map2bmp.o util.o -o $@

.c.o:
	$(CC) $(CFLAGS) $(DEFINES) -MMD -c $< -o $*.o

clean:
	rm -f *.o *.d

-include map2bmp.d
