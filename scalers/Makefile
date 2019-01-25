
all: scaler_nearest.so scaler_tv2x.so scaler_xbr.so

CPPFLAGS += -O2 -fPIC -Iscale2x-4.0/ -Ihqx/include/ -DNDEBUG -DUSE_STATIC_SCALER

OBJS_SCALER_NEAREST := scaler_nearest.o

scaler_nearest.so: $(OBJS_SCALER_NEAREST)
	$(CXX) $(LDFLAGS) -fPIC -shared -o $@ $^

OBJS_SCALER_TV2X := scaler_tv2x.o

scaler_tv2x.so: $(OBJS_SCALER_TV2X)
	$(CXX) $(LDFLAGS) -fPIC -shared -o $@ $^

OBJS_SCALER_XBR := scaler_xbr.o

scaler_xbr.so: $(OBJS_SCALER_XBR)
	$(CXX) $(LDFLAGS) -fPIC -shared -o $@ $^

clean:
	rm -f $(OBJS_SCALER_NEAREST) $(OBJS_SCALER_TV2X) $(OBJS_SCALER_XBR) *.so
