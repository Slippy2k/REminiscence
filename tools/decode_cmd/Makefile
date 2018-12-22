
CXXFLAGS += -g -O -Wall -Wpedantic -MMD

SRCS = decode_cmd.cpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

decode_cmd: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS)

clean:
	rm -f $(OBJS) $(DEPS) decode_cmd

-include $(DEPS)
