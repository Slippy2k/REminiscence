
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static uint8_t reverseBits(uint8_t ch) {
	uint8_t r = 0;
	for (int b = 0; b < 8; ++b) {
		if (ch & (1 << b)) {
			r |= (1 << (7 - b));
		}
	}
	return r;
}

static uint8_t decryptChar(uint8_t ch) {
	return reverseBits(ch ^ 0x55);
}

static void dumpBinary(FILE *fp, const int pos, const int end) {
	int count = 0;

	fseek(fp, pos, SEEK_SET);
	while (ftell(fp) <= end) {
		if ((count % 16) == 0) {
			printf("\n\t");
		} else {
			printf(" ");
		}
		printf("0x%02X,", fgetc(fp));
		++count;
	}
}

/* FlashBack Amiga_EN _creditsData */
static const int kPos = 0x29D48;
static const int kEnd = 0x2A6F0;

/* Flashback DOS_SSI _protectionData */
static void dumpProtectionData(FILE *fp) {
	static const int kPos = 0x1F612;
	static const int kLen = 720;

	dumpBinary(fp, kPos, kPos + kLen - 1);
	{
		int i, j, count;
		uint8_t data[kLen];

		fseek(fp, kPos, SEEK_SET);
		count = fread(data, 1, kLen, fp);
		for (i = 0; i < count; i += 18) {
			// 0 : page number
			// 1 : column number
			// 2 : line number
			// 3 : word number
			fprintf(stdout, "page %d column %d line %2d word %d : ", data[i], data[i + 1], data[i + 2], data[i + 3]);
			for (j = 4; j < 18; ++j) {
				const uint8_t ch = decryptChar(data[i + j]);
				if (!(ch >= 'A' && ch <= 'Z')) {
					break;
				}
				fprintf(stdout, "%c", ch);
			}
			fprintf(stdout, "\n");
		}
	}
}

/* Flashback DOS_SSI _protectionText */
static void dumpProtectionText(FILE *fp) {
	static const int kPos = 0x1F8E2;
	static const int kLen = 152;

	dumpBinary(fp, kPos, kPos + kLen - 1);
	{
		int i;

		fseek(fp, kPos, SEEK_SET);
		for (i = 0; i < kLen; ++i) {
			const uint8_t ch = decryptChar(fgetc(fp));
			fprintf(stdout, "%c", ch);
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc >= 2) {
		int count = 0;
		FILE *fp = fopen(argv[1], "rb");
		if (fp) {
			dumpProtectionData(fp);
			dumpProtectionText(fp);
			fclose(fp);
		}
	}
	return 0;
}
