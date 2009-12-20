
#include <cstdio>
#include <cstring>
#include <cassert>
#define SYS_LITTLE_ENDIAN
#include "../../sys.h"

#include "musicdata.h"

const uint8 *_snd_ingameMusicOffset = 0;
uint16 _sfx_curOrder, _sfx_length;
uint16 word_304A6, word_304A8, word_304AA;

const uint16 _sfx_periodTable[] = {
	0x434, 0x3F8, 0x3C0, 0x38A, 0x358, 0x328, 0x2FA, 0x2D0, 0x2A6, 0x280,
	0x25C, 0x23A, 0x21A, 0x1FC, 0x1E0, 0x1C5, 0x1AC, 0x194, 0x17D, 0x168,
	0x153, 0x140, 0x12E, 0x11D, 0x10D, 0x0FE, 0x0F0, 0x0E2, 0x0D6, 0x0CA,
	0x0BE, 0x0B4, 0x0AA, 0x0A0, 0x097, 0x08F, 0x087, 0x07F, 0x078, 0x071
};

void music_vector(const uint8 *p) { // 50Hz
//	if (_snd_ingameMusicToPlay == 0) goto loc_11948;
	const uint16 *a2 = _sfx_periodTable;
	const uint8 *a1 = _snd_ingameMusicOffset;
	if (a1 == 0) {
		a1 = p + 0x5E - 15 * 4; // 0x22
		_sfx_curOrder = 0;
		_sfx_length = 0;
	}
	if (_sfx_length != 0) {
		--_sfx_length;
		goto loc_11948;
	} else {
		uint16 tmp = READ_BE_UINT16(p + 0x3C - 15 * 4); // 0x0, numOrders
		_sfx_length = READ_BE_UINT16(p + 0x3E - 15 * 4); // 0x2, orderDelay
		printf("mod[0x3C]=%d mod[0x3E]=%d\n", tmp, _sfx_length);
		uint16 d1, d2; // XXX default value ?
		uint8 d0;
		
		d0 = *a1++; // sample_num for channel 0
		if (d0 != 0) {
			d1 = READ_BE_UINT16(p + (d0 - 1) * 2 + 0x40 - 15 * 4);
//			d6 = dword_3048E = p[(d0 - 1) * 4]; // sample
		}
		printf("d0=%d\n", d0);
		d0 = *a1++;
		if (d0 != 0) {
			d0 = (d0 - 1) + d1;
			assert(d0 < 40);
			word_304A6 = d2 = _sfx_periodTable[d0]; // READ_BE_UINT16(a2 + d0 * 2);
//			d0 = 0;
//			startIngameMusicSample();
		}
		printf("d0=%d d1=%d word_304A6=0x%X\n", d0, d1, word_304A6);
		
		d0 = *a1++; // sample_num for channel 1
		if (d0 != 0) {
			d1 = READ_BE_UINT16(p + (d0 - 1) * 2 + 0x40 - 15 * 4);
//			d6 = dword_30492 = p[(d0 - 1) * 4]; // sample
		}
		printf("d0=%d\n", d0);
		d0 = *a1++;
		if (d0 != 0) {
			d0 = (d0 - 1) + d1;
			assert(d0 < 40);
			word_304A8 = d2 = _sfx_periodTable[d0]; // READ_BE_UINT16(a2 + d0 * 2);
//			d0 = 1;
//			startIngameMusicSample();
		}
		printf("d0=%d d1=%d word_304A8=0x%X\n", d0, d1, word_304A8);
		
		d0 = *a1++; // sample_num for channel 2
		if (d0 != 0) {
			d1 = READ_BE_UINT16(p + (d0 - 1) * 2 + 0x40 - 15 * 4);
//			d6 = dword_30496 = p[(d0 - 1) * 4]; // sample
		}
		printf("d0=%d\n", d0);
		d0 = *a1++;
		if (d0 != 0) {
			d0 = (d0 - 1) + d1;
			assert(d0 < 40);
			word_304AA = d2 = _sfx_periodTable[d0]; // READ_BE_UINT16(a2 + d0 * 2);
//			d0 = 2;
//			startIngameMusicSample();
		}
		printf("d0=%d d1=%d word_304AA=%d", d0, d1, word_304AA);
		
		_snd_ingameMusicOffset = a1;
		d0 = _sfx_curOrder + 1;
		if (d0 >= READ_BE_UINT16(p + 0x3C - 15 * 4)) { // 0x0
//			_snd_ingameMusicToPlay = 0;
			_sfx_length += 20;
		}
		_sfx_curOrder = d0;
		return;
	}
//loc_11948:
// 	if (_snd_ingameMusicToPlay == 0 && _snd_ingameMusicOffset != 0) {
//		d0 = _sfx_length;
//		if (d0 != 0) {
//			_sfx_length = d0 - 1;
//		} else {
//			_snd_ingameMusicOffset = 0;
//			0xDFF096 = 7;
//		}
//	}
}

int main(int argc, char *argv[]) {
	const uint8 *p = _snd_musicData68_69;
	init(p);
	return 0;	
}
	





/*
d0 = channel
d2 = period
d6 = sound_data
void Sound::startIngameMusicSample() {
	d1 = 1 << d0;
	*(uint16 *)0xDFF096 = d1;
	a1 = 0xDFF0A0 + 4 * d0;
	d0 = 600;
	while (--d0 != 0);
	a0 = d6; // sample data
	d0 = READ_BE_UINT16(a0); a0 += 2; // sample len
	d3 = READ_BE_UINT16(a0); a0 += 2; // sample volume
	d7 = READ_BE_UINT16(a0); a0 += 2; // loop position
	d5 = READ_BE_UINT16(a0); a0 += 2; // loop len
	d6 = a0;
	*(uint16 *)a0 = 0;
	*(uint32 *)(a1 + 0) = d6; // data
	*(uint16 *)(a1 + 4) = d0 >> 1; // len
	*(uint16 *)(a1 + 6) = d2; // period
	*(uint16 *)(a1 + 8) = d3; // volume
	d1 |= -0x8000;
	*(uint16 *)0xDFF096 = d1;
	d0 = 256;
	while (--d0 != 0);	
	d5 >>= 1;
	d6 += d7;
	*(uint32 *)(a1 + 0) = d6;
	*(uint16 *)(a1 + 4) = d5;
	*(uint16 *)(a1 + 6) = d2;
	*(uint16 *)(a1 + 8) = d3;
}
*/
