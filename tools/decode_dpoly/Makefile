
CC = gcc
CFLAGS = -g -O -Wall -Wuninitialized -Wno-multichar

decode_dpoly: decode_dpoly.o
	$(CC) $(LDFLAGS) decode_dpoly.o -o $@
	
.c.o:
	$(CC) $(CFLAGS) -MMD -c $< -o $*.o

clean:
	rm -f *.o *.d

-include decode_dpoly.d
