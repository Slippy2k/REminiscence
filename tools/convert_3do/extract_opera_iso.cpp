
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include "endian.h"

static const bool kDumpGameData = false;

static const char *GAMEDATA_DIRECTORY = "GameData";

static void dumpGameData(const char *name, FILE *in, uint32_t offset, uint32_t size) {
	fprintf(stdout, "%s at 0x%x size %d bytes\n", name, offset, size);
	// makeDirectory(GAMEDATA_DIRECTORY);
	char path[MAXPATHLEN];
	snprintf(path, sizeof(path), "%s/%s", GAMEDATA_DIRECTORY, name);
	FILE *out = fopen(path, "wb");
	if (out) {
		uint8_t buf[4096];
		int count;
		fseek(in, offset, SEEK_SET);
		while ((count = fread(buf, 1, size < sizeof(buf) ? size : sizeof(buf), in)) > 0) {
			fwrite(buf, 1, count, out);
			size -= count;
		}
		fclose(out);
	}
}

static const int BLOCK_SIZE = 2048;

static void readIso(FILE *fp, int block, int flags) {
	uint32_t attr = 0;
	do {
		fseek(fp, block * BLOCK_SIZE + 20, SEEK_SET);
		do {
			uint8_t buf[72];
			fread(buf, sizeof(buf), 1, fp);
			attr = READ_BE_UINT32(buf);
			const char *name = (const char *)buf + 32;
			const uint32_t count = READ_BE_UINT32(buf + 64);
			const uint32_t offset = READ_BE_UINT32(buf + 68);
			fprintf(stdout, "'%s' offset 0x%x count %d attr 0x%x\n", name, offset, count, attr);
			fseek(fp, count * 4, SEEK_CUR);
			switch (attr & 255) {
			case 2:
				if (kDumpGameData && (flags & 1) != 0) {
					const int pos = ftell(fp);
					dumpGameData(name, fp, offset * BLOCK_SIZE, READ_BE_UINT32(buf + 16));
					fseek(fp, pos, SEEK_SET);
				}
				break;
			case 7:
				if (!kDumpGameData || strcmp(name, GAMEDATA_DIRECTORY) == 0) {
					const int pos = ftell(fp);
					readIso(fp, offset, 1);
					fseek(fp, pos, SEEK_SET);
				}
				break;
			}
		} while (attr != 0 && attr < 256);
		++block;
	} while ((attr >> 24) == 0x40);
}

static int extractOperaIso(FILE *fp) {
	uint8_t buf[128];
	fread(buf, sizeof(buf), 1, fp);
	if (buf[0] == 1 && memcmp(buf + 40, "CD-ROM", 6) == 0) {
		const int offset = READ_BE_UINT32(buf + 100);
		readIso(fp, offset, 0);
	}
	return 0;
}

int main(int argc, char *argv[]) {
	if (argc == 2) {
		FILE *fp = fopen(argv[1], "rb");
		if (fp) {
			extractOperaIso(fp);
			fclose(fp);
		}
	}
	return 0;
}
