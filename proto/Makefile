
SDL_CFLAGS = `sdl2-config --cflags`
SDL_LIBS   = `sdl2-config --libs` -lSDL2_mixer

GL_LIBS = -lGL

CXX = g++
CXXFLAGS += -g -O -Wall -Wuninitialized -Wno-sign-compare -MMD $(SDL_CFLAGS)

SRCS_LIB = collision.cpp decode.cpp file.cpp game.cpp piege.cpp resource_data.cpp resource_mac.cpp saveload.cpp scaler.cpp staticres.cpp stub.cpp util.cpp
OBJS_LIB = $(SRCS_LIB:.cpp=.o)
DEPS_LIB = $(SRCS_LIB:.cpp=.d)

SRCS_EXE = main.cpp mixer.cpp input.cpp render.cpp
OBJS_EXE = $(SRCS_EXE:.cpp=.o)
DEPS_EXE = $(SRCS_EXE:.cpp=.d)

all: fb

libfb.so: $(OBJS_LIB)
	$(CXX) $(LDFLAGS) -shared -o $@ $(OBJS_LIB) $(GL_LIBS) -lz

fb: $(OBJS_EXE) $(OBJS_LIB)
	$(CXX) $(LDFLAGS) -o $@ $^ -ldl $(SDL_LIBS) $(GL_LIBS) -lz

clean:
	rm -f *.o *.d libfb.so fb

-include $(DEPS_EXE) $(DEPS_LIB)
