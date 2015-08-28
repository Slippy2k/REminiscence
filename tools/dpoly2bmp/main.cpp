
#include "dpoly.h"

int main(int argc, char *argv[]) {
	if (argc == 2) {
		DPoly *dp = new DPoly;
		dp->Decode(argv[1]);
		delete dp;
	} else {
		printf("Syntax: %s FILE.SET\n", argv[0]);
	}
	return 0;	
}
