
CXX = g++
CXXFLAGS = -g -O -Wall -Wuninitialized

SRCS = bitmap.cpp dpoly.cpp file.cpp graphics.cpp main.cpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

dpoly2bmp: $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $@

.cpp.o:
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $*.o

clean:
	rm -f *.o *.d *.BMP *.RGBA *.TGA *.TXT

-include $(DEPS)
