
SDL_CFLAGS = `sdl-config --cflags`
SDL_LIBS = `sdl-config --libs`

GL_LIBS = -lGL -lGLU
#GL_LIBS = -lopengl32 -lglu32

DEFINES = -DSYS_LITTLE_ENDIAN

CXX = g++
CXXFLAGS:= -g -O -Wall -Wuninitialized -Wundef -Wreorder
CXXFLAGS+= $(SDL_CFLAGS) $(DEFINES)

SRCS = cutscene.cpp main.cpp oglstub.cpp staticres.cpp util.cpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

fbply: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(SDL_LIBS) $(GL_LIBS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $*.o

clean:
	rm -f *.o *.d

-include $(DEPS)
