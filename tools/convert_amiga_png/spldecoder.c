
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "tables.h"

static struct {
	int offset[100];
	int size[100];
} _dataTable;

#define PAULA_FREQ 3546897

static void load_spl(const unsigned char *d3, unsigned char *d4) {
	int i, offset = 0;

	memset(_dataTable.offset, 0, 90 * sizeof(int)); // a3
	memset(_dataTable.size,   0, 90 * sizeof(int)); // a4
#if 0
	if (_currentLevel == 4 || _currentLevel == 6) {
CODE:0000FFA2                 lea     (loc_B7A2).l,a1
CODE:0000FFA8                 adda.l  #$21C,a1
CODE:0000FFAE                 move.l  #loc_389C,d0
CODE:0000FFB4                 addi.l  #100,d0
CODE:0000FFBA                 move.l  d0,(a1)
	}
#endif
#if 0
	d3[64] = 0; // d3 == a2
	a0 = _res_splForLevel[_currentLevel];
#endif
	for (i = 0; i < 66; ++i) {
		int d0 = d3[i];
		if (d0 != 0) {
			d0 = (d4[offset] << 8) | d4[offset + 1]; offset += 2;
			if ((d0 & 0x8000) == 0) {
				++d0;
				d0 &= ~1;
				_dataTable.offset[i] = offset;
				_dataTable.size[i] = d0;
				offset += d0;
				continue;
			}
		} else {
			d0 = (d4[offset] << 8) | d4[offset + 1]; offset += 2;
		}
		if ((d0 & 0x8000) == 0) {
			++d0;
			d0 &= ~1;
			offset += d0;
		}
		_dataTable.offset[i] = 0;
		_dataTable.size[i] = 0;
	}
}	

static void play_sound(const unsigned char *spl_table, int d7) {
	int d0;

	if (d7 < 67 || d7 > 75) {
		d7 = spl_table[d7];
		if (d7 == 0) {
			return;
		}
		--d7;
	}
	d0 = _dataTable.offset[d7];
	if (d0 == 0) {
		return;
	} else if (d0 < 0) {
//		_snd_ingameMusicToPlay = -d0;
	} else {
#if 0
		aud0lch = sample + _dataTable[d0].offset;
		aud0len = _dataTable[d7].size >> 1;
		aud0per = 650;	
		aud0vol = d2;
#endif
	}
}

static unsigned char *load_file(const char *filepath) {
	unsigned char *ptr = NULL;
	int sz;
	FILE *fp = fopen(filepath, "r");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		sz = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		ptr = malloc(sz);
		if (ptr) {
			fread(ptr, sz, 1, fp);
		}
		fclose(fp);
	}
	return ptr;
}

int main(int argc, char *argv[]) {
	int i;

	unsigned char *_spl_file_buffer = load_file("../../DATA/DATA_amiga/data/level1.spl");
	load_spl(_spl_1_table, _spl_file_buffer);
	for (i = 0; i < 90; ++i) {
		if (_dataTable.size[i] != 0) {
			printf("sound %2d offset 0x%05X size %4d freq %4d\n", i, _dataTable.offset[i], _dataTable.size[i], PAULA_FREQ / 650);
		}
	}
	return 0;
}

