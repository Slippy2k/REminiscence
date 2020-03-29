
CXXFLAGS += -g -O -Wall -Wpedantic -MMD

SRCS = decode_cmd.cpp decode_pol.cpp

all: decode_cmd decode_pol

decode_cmd: decode_cmd.o
	$(CXX) $(LDFLAGS) -o $@ $^

decode_pol: bitmap.o decode_pol.o file.o graphics.o
	$(CXX) $(LDFLAGS) -o $@ $^

clean:
	rm -f *.o decode_cmd decode_pol
