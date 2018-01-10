
#include <cstdio>
#include <cstring>
#include <cassert>
#include "intern.h"

static uint16_t READ_BE_UINT16(const uint8_t *p) {
	return (p[0] << 8) | p[1];
}

static uint32_t fileReadUint32BE(FILE *fp) {
	uint8_t a = fgetc(fp);
	uint8_t b = fgetc(fp);
	uint8_t c = fgetc(fp);
	uint8_t d = fgetc(fp);
	return (d << 24) | (c << 16) | (b << 8) | a;
}

static void dumpHex(FILE *fp, uint32_t off1, uint32_t off2, const char *name, bool isModule) {
	fseek(fp, off1, SEEK_SET);
	if (isModule) {
		// skip samples
		for (int s = 0; s < 15; ++s) {
			fileReadUint32BE(fp);
		}
	}
	uint8_t lastByte = 0;
	printf("const uint8_t %s[] = {\n\t", name);
	int i = 0;
	while (ftell(fp) < (int)off2) {
		if (i == 16) {
			printf("\n\t");
			i = 0;
		}
		lastByte = fgetc(fp);
		printf("0x%02X,", lastByte);
		++i;
		if (i != 16) {
			printf(" ");
		}
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

static void dumpRaw(FILE *fp, uint32_t offset, uint32_t endOffset, int sampleNumber) {
	char name[64];
	snprintf(name, sizeof(name), "sample%d.raw", sampleNumber);
	FILE *out = fopen(name, "wb");
	if (out) {
		fseek(fp, offset, SEEK_SET);
		uint8_t header[8];
		const int count = fread(header, 1, sizeof(header), fp);
		const int len = READ_BE_UINT16(header);
		const int volume = READ_BE_UINT16(header + 2);
		const int loopOffset = READ_BE_UINT16(header + 4);
		const int loopLen = READ_BE_UINT16(header + 6);
		while (ftell(fp) < endOffset) {
			fputc(fgetc(fp), out);
		}
		fclose(out);
	}
}

struct Module {
	const char *name;
	uint32_t offs_start;
	uint32_t offs_end;
	bool module;
} MODULES[] = {
	/* module data */
	{ "SfxPlayer::_musicData68_69", 0xC30E, 0xC30E + 4 * 15 + 34 + 6 * 0x1B, true },
	{ "SfxPlayer::_musicDataUnk",   0xC40E, 0xC40E + 4 * 15 + 34 + 6 * 0x25, true },
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

static const int MODULES_COUNT = sizeof(MODULES) / sizeof(MODULES[0]);

#undef main
int main(int argc, char* argv[]) {
	FILE *fp = fopen("flashback", "rb");
	if (fp) {
		for (size_t i = 0; i < MODULES_COUNT; ++i) {
			dumpHex(fp, MODULES[i].offs_start, MODULES[i].offs_end, MODULES[i].name, MODULES[i].module);
		}
		int sampleNumber = 0;
		for (size_t i = 0; i < MODULES_COUNT; ++i) {
			if (!MODULES[i].module) {
				dumpRaw(fp, MODULES[i].offs_start, MODULES[i].offs_end, sampleNumber);
				++sampleNumber;
			}
		}
		fclose(fp);
	}
	return 0;
}
