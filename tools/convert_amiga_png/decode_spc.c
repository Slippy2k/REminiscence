
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "unpack.h"
#include "util.h"

void decode_spc(unsigned char *p, int size) {
	int i, offset, offset2, count;
	const unsigned char *hdr;

	count = movew(p) / 2;
	printf("SPC count %d\n", count);
	for (i = 0; i < count; ++i) {
		offset = movew(p + i * 2);
		offset2 = movew(p + i * 2 + 2);
		// 0: mbk/rpc index
		// 1: x
		// 2: y
		// 3-4-5-6: unk
		// 8: count
		hdr = p + offset;
		assert(hdr[0] < 0x4A);
		printf("i %d offset 0x%X mbk %d x %d y %d count %d\n", i, offset, hdr[0], hdr[1], hdr[2], hdr[8]);
		printf("next offset 0x%X\n", offset + 9 + hdr[8] * 4);
	}
}

