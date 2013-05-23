
CXXFLAGS += -g -MMD -Wall `sdl-config --cflags`

OBJS = cutscene.o collision.o file.o game.o gl_texture.o graphics.o main.o menu.o mixer.o piege.o resource.o saveload.o sfx_player.o staticres.o stub.o unpack.o util.o video.o
DEPS = $(OBJS:.o=.d)

fb: $(OBJS)
	$(CXX) -o $@ $^ `sdl-config --libs` -lGL

clean:
	rm -f *.d *.o

-include $(DEPS)
