
SDL_CFLAGS = `sdl-config --cflags`
SDL_LIBS = `sdl-config --libs`

DEFINES = -DBYPASS_PROTECTION
#DEFINES = -DBYPASS_PROTECTION -DENABLE_PASSWORD_MENU -DNDEBUG

CXX = g++
CXXFLAGS := -g -O -Wall -Wuninitialized -Wshadow -Wimplicit -Wundef -Wreorder -Wnon-virtual-dtor -Wno-multichar
CXXFLAGS += -MMD $(SDL_CFLAGS) -DUSE_ZLIB $(DEFINES)

SRCS = collision.cpp cutscene.cpp file.cpp game.cpp graphics.cpp main.cpp menu.cpp \
	mixer.cpp mod_player.cpp piege.cpp resource.cpp scaler.cpp sfx_player.cpp \
	staticres.cpp systemstub_sdl.cpp unpack.cpp util.cpp video.cpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

-include Makefile.local

rs: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(SDL_LIBS) -lz

clean:
	rm -f *.o *.d

-include $(DEPS)
