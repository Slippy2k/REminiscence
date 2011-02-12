
SDL_CFLAGS = `sdl-config --cflags`
SDL_LIBS   = `sdl-config --libs` -lGL

CXX = g++
#CXX = i586-mingw32msvc-g++
CXXFLAGS += -g -O -Wall -Wuninitialized -Wno-sign-compare $(SDL_CFLAGS)

SRCS_LIB = collision.cpp decode.cpp file.cpp game.cpp piege.cpp resource_data.cpp resource_mac.cpp saveload.cpp staticres.cpp stub.cpp util.cpp
OBJS_LIB = $(SRCS_LIB:.cpp=.o)
DEPS_LIB = $(SRCS_LIB:.cpp=.d)

all: libfb.so fb

libfb.so: $(OBJS_LIB)
	$(CXX) $(LDFLAGS) -shared -o $@ $(OBJS_LIB) -lGL -lz

fb: main.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(SDL_LIBS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $*.o

clean:
	rm -f *.o *.d libfb.so fb

-include $(DEPS_LIB) main.d
