
#include <cstdio>
#include "seq.h"
#include "systemstub.h"


static const char *USAGE = 
	"seqply - Tiertex Video Sequence Player\n"
	"Usage: seqply [video.seq]\n";

#undef main
int main(int argc, char *argv[]) {
	if (argc == 2) {
	  	g_debugMask = 0; //DBG_INFO | DBG_SEQ;
		SystemStub *stub = SystemStub_SDL_create();
		Seq *seq = new Seq(stub, argv[1]);
		seq->start();
		delete seq;
		delete stub;
	} else {
		printf(USAGE);
	}
	return 0;
}
