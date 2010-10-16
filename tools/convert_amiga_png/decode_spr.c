
#include <stdio.h>
#include <assert.h>

void decode_spr(unsigned char *data, int data_size) {
	int w, h, count;

	while (data_size > 0) {
		w = *data++;
		++w;
		h = *data++;
		++h;
		count = w * h * 8 + 4;
		data += count;
		printf("icon %dx%d count %d\n", w, h, count);
		data_size -= count + 2;
	}
	assert(data_size == 0);
}

