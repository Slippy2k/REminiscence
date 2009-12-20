
#include "dpoly.h"

int main(int argc, char *argv[]) {
	if (argc == 2) {
		DPoly().Decode(argv[1]);
	} else {
		printf("Syntax: %s FILE.SET\n", argv[0]);
	}
	return 0;	
}
