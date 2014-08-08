
SDL_CFLAGS = `sdl-config --cflags`
SDL_LIBS   = `sdl-config --libs`

GL_LIBS = -lGL

CXX = g++
CXXFLAGS += -g -fPIC -O -Wall -Wuninitialized -Wno-sign-compare -MMD $(SDL_CFLAGS)

SRCS_LIB = collision.cpp decode.cpp file.cpp game.cpp piege.cpp resource_data.cpp resource_mac.cpp saveload.cpp scaler.cpp staticres.cpp stub.cpp util.cpp
OBJS_LIB = $(SRCS_LIB:.cpp=.o)
DEPS_LIB = $(SRCS_LIB:.cpp=.d)

all: libfb.so fb

libfb.so: $(OBJS_LIB)
	$(CXX) $(LDFLAGS) -shared -o $@ $(OBJS_LIB) $(GL_LIBS) -lz

fb: main.o
	$(CXX) $(LDFLAGS) -o $@ $^ -ldl $(SDL_LIBS) $(GL_LIBS)

clean:
	rm -f *.o *.d libfb.so fb

-include $(DEPS_LIB) main.d
