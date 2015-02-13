
#include <cstdio>
#include <cstring>
#include <cassert>
#include "sys.h"

static uint32 read_u32(FILE *fp) {
	uint8 a = fgetc(fp);
	uint8 b = fgetc(fp);
	uint8 c = fgetc(fp);
	uint8 d = fgetc(fp);
	return (d << 24) | (c << 16) | (b << 8) | a;
}

static void dump(FILE *fp, uint32 off1, uint32 off2, const char *name, bool isModule) {
	fseek(fp, off1, SEEK_SET);
	if (isModule) {
		// skip samples
		for (int s = 0; s < 15; ++s) {
			read_u32(fp);
		}
	}
	uint8 lastByte = 0;
	printf("const uint8 %s[] = {\n\t", name);
	int i = 0;
	while (ftell(fp) < (int)off2) {
		if (i == 16) {
			printf("\n\t");
			i = 0;
		}
		lastByte = fgetc(fp);
		printf("0x%02X, ", lastByte);
		++i;
	}
	if (!isModule) {
		if (i == 16) {
			printf("\n\t");
		}
		// duplicate last byte
		printf("0x%02X", lastByte);
	}
	printf("\n};\n");
	printf("// sizeof() = %d\n", int(off2 - off1));
}

struct Module {
	const char *name;
	uint32 offs_start;
	uint32 offs_end;
	bool module;
} MODULES[] = {
	/* module data */
	{ "SfxPlayer::_musicData68_69", 0xC30E, 0xC30E + 4 * 15 + 34 + 6 * 0x1B, true },
//	{ "SfxPlayer::_musicDataUnk",   0xC40E, 0xC40E + 4 * 15 + 34 + 6 * 0x25, true },
	{ "SfxPlayer::_musicData70_71", 0xC54A, 0xC54A + 4 * 15 + 34 + 6 * 0x12, true },
	{ "SfxPlayer::_musicData72",    0xC614, 0xC614 + 4 * 15 + 34 + 6 * 0x4F, true },
	{ "SfxPlayer::_musicData73",    0xC84C, 0xC84C + 4 * 15 + 34 + 6 * 0x41, true },
	{ "SfxPlayer::_musicData74",    0xCA30, 0xCA30 + 4 * 15 + 34 + 6 * 0x41, true },
	{ "SfxPlayer::_musicData75",    0xCC14, 0xCC14 + 4 * 15 + 34 + 6 * 0x41, true },
	/* sample data */
	{ "SfxPlayer::_musicDataSample1", 0x1F978, 0x1F978 + 8 + 0x082C, false },
	{ "SfxPlayer::_musicDataSample2", 0x201B8, 0x201B8 + 8 + 0x0D90, false },
	{ "SfxPlayer::_musicDataSample3", 0x20F50, 0x20F50 + 8 + 0x118C, false },
	{ "SfxPlayer::_musicDataSample4", 0x220E4, 0x220E4 + 8 + 0x01F8, false },
	{ "SfxPlayer::_musicDataSample5", 0x222E4, 0x222E4 + 8 + 0x09E4, false },
	{ "SfxPlayer::_musicDataSample6", 0x22CD0, 0x22CD0 + 8 + 0x092E, false },
	{ "SfxPlayer::_musicDataSample7", 0x23606, 0x23606 + 8 + 0x0346, false },
	{ "SfxPlayer::_musicDataSample8", 0x23B06, 0x23B06 + 8 + 0x0002, false },
};

#undef main
int main(int argc, char* argv[]) {
	FILE *fp = fopen("flashback", "rb");
	if (fp) {
		for (size_t i = 0; i < sizeof(MODULES)/sizeof(MODULES[0]); ++i) {
			dump(fp, MODULES[i].offs_start, MODULES[i].offs_end, MODULES[i].name, MODULES[i].module);
		}
		fclose(fp);
	}
	return 0;
}
