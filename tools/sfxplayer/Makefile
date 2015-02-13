
SDL_CFLAGS = `sdl-config --cflags`
SDL_LIBS = `sdl-config --libs`

DEFINES = -DSYS_LITTLE_ENDIAN

CXX = g++
CXXFLAGS:= -g -O -Wall
CXXFLAGS+= $(SDL_CFLAGS) $(DEFINES)

SRCS_SFXPLY = sfx_player.cpp util.cpp
OBJS_SFXPLY = $(SRCS_SFXPLY:.cpp=.o)
DEPS_SFXPLY = $(SRCS_SFXPLY:.cpp=.d)

SRCS_DUMP = dump.cpp
OBJS_DUMP = $(SRCS_DUMP:.cpp=.o)
DEPS_DUMP = $(SRCS_DUMP:.cpp=.d)

all: sfxdump sfxply

sfxply: $(OBJS_SFXPLY)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS_SFXPLY) $(SDL_LIBS)

sfxdump: $(OBJS_DUMP)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS_DUMP) $(SDL_LIBS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $*.o

clean:
	rm -f *.o *.d

-include $(DEPS_SFXPLY)
