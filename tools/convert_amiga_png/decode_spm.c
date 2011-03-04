
#include <stdio.h>
#include <assert.h>
#include "unpack.h"
#include "util.h"

static unsigned char spmData[256 * 256];

void decode_spm(unsigned char *data, int data_size) {
	int size, ret;

	size = movel(data);
	printf("spm size %d\n", size);
	assert(size + 4 == data_size);
	ret = delphine_unpack(data + size, spmData);
	assert(ret);
//	dump_file(0, spmData, movel(data + size));
}

