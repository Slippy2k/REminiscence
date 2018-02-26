
all: scaler_epx.so scaler_tv2x.so scaler_xbrz.so

CPPFLAGS += -O2 -fPIC -Iscale2x-4.0/ -Ihqx/include/

OBJS_SCALER_EPX := scaler_epx.o scale2x-4.0/scale2x.o scale2x-4.0/scale3x.o scale2x-4.0/scalebit.o

scaler_epx.so: $(OBJS_SCALER_EPX)
	$(CXX) $(LDFLAGS) -fPIC -shared -o $@ $^

OBJS_SCALER_TV2X := scaler_tv2x.o

scaler_tv2x.so: $(OBJS_SCALER_TV2X)
	$(CXX) $(LDFLAGS) -fPIC -shared -o $@ $^

OBJS_SCALER_XBRZ := scaler_xbrz.o xbrz/xbrz.o

scaler_xbrz.so: $(OBJS_SCALER_XBRZ)
	$(CXX) $(LDFLAGS) -fPIC -shared -o $@ $^

clean:
	rm -f $(OBJS_SCALER_EPX) $(OBJS_SCALER_TV2X) $(OBJS_SCALER_XBRZ) *.so
