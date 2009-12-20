
SDL_CFLAGS = `sdl-config --cflags`
SDL_LIBS = `sdl-config --libs`

DEFINES = -DSYS_LITTLE_ENDIAN

CXX = g++
CXXFLAGS:= -g -O -Wall -Wuninitialized -Wno-unknown-pragmas -Wshadow -Wimplicit 
CXXFLAGS+= -Wundef -Wreorder -Wwrite-strings -Wnon-virtual-dtor -Wno-multichar
CXXFLAGS+= $(SDL_CFLAGS) $(DEFINES)

SRCS = main.cpp sdlstub.cpp seq.cpp util.cpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

seqply: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(SDL_LIBS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $*.o

clean:
	rm -f *.o *.d

-include $(DEPS)








