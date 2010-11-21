
SDL_CFLAGS = `sdl-config --cflags`
SDL_LIBS   = `sdl-config --libs` -lGL

CXX = g++
CXXFLAGS += -g -O -Wall -Wuninitialized -Wno-sign-compare $(SDL_CFLAGS)

SRCS = collision.cpp decode.cpp file.cpp game.cpp main.cpp piege.cpp resource_data.cpp resource_mac.cpp staticres.cpp util.cpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

OUT = fb

$(OUT): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(SDL_LIBS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $*.o

clean:
	rm -f $(OBJS) $(DEPS) $(OUT)

-include $(DEPS)
