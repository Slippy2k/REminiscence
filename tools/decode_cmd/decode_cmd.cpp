
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>

enum {
	MAX_FILESIZE = 0x10000,
};

static uint8_t _fileBuf[MAX_FILESIZE];

static FILE *_out = stdout;

static uint16_t readWord(const uint8_t *p) {
	return (p[0] << 8) | p[1]; // BE
}

enum {
	/* 0x00 */
	op_markCurPos,
	op_refreshScreen,
	op_waitForSync,
	op_drawShape,
	/* 0x04 */
	op_setPalette,
	op_markCurPos2,
	op_drawStringAtBottom,
	op_nop,
	/* 0x08 */
	op_skip3,
	op_refreshAll,
	op_drawShapeScale,
	op_drawShapeScaleRotate,
	/* 0x0C */
	op_drawCreditsText,
	op_drawStringAtPos,
	op_handleKeys
};

static void printOpcode(uint16_t addr, uint8_t opcode, int args[16]) {
	fprintf(_out, "%04X: ", addr);
	switch (opcode) {
	case op_markCurPos:
		fprintf(_out, "op_markCurPos");
		break;
	case op_refreshScreen:
		fprintf(_out, "op_refreshScreen %d", args[0]);
		break;
	case op_waitForSync:
		fprintf(_out, "op_waitForSync");
		break;
	case op_drawShape:
		fprintf(_out, "op_drawShape shape:%d", args[0]);
		break;
	case op_setPalette:
		fprintf(_out, "op_setPalette src:%d dst:%d", args[0], args[1]);
		break;
	case op_markCurPos2:
		fprintf(_out, "op_markCurPos2");
		break;
	case op_drawStringAtBottom:
		fprintf(_out, "op_drawStringAtBottom id:%d", args[0]);
		break;
	case op_nop:
		fprintf(_out, "op_nop");
		break;
	case op_skip3:
		fprintf(_out, "op_skip3 %d %d %d", args[0], args[1], args[2]);
		break;
	case op_refreshAll:
		fprintf(_out, "op_refreshAll");
		break;
	case op_drawShapeScale:
		fprintf(_out, "op_drawShapeScale");
		break;
	case op_drawShapeScaleRotate:
		fprintf(_out, "op_drawShapeScaleRotate");
		break;
	case op_drawCreditsText:
		fprintf(_out, "op_drawCreditsText");
		break;
	case op_drawStringAtPos:
		fprintf(_out, "op_drawStringAtPos id:%d", args[0]);
		break;
	case op_handleKeys:
		fprintf(_out, "op_handleKeys");
		break;
	}
	fprintf(_out, "\n");
}

static void (*visitOpcode)(uint16_t addr, uint8_t opcode, int args[16]);

static int parse(const uint8_t *buf, uint32_t size) {	
	const uint8_t *p = buf;
	while (p < buf + size) {
		uint16_t addr = p - buf;
		int a, b, c, d, e, f;
		uint8_t op = *p++;
		if (op & 0x80) {
			break;
		}
		a = b = c = d = e = f = 0;
		op >>= 2;
		switch (op) {
		case op_markCurPos:
			break;
		case op_refreshScreen:
			a = *p++;
			break;
		case op_waitForSync:
			a = *p++;
			break;
		case op_drawShape:
			a = readWord(p); p += 2;
			if (a & 0x8000) {
				b = (int16_t)readWord(p); p += 2;
				c = (int16_t)readWord(p); p += 2;
			}
			break;
		case op_setPalette:
			a = *p++;
			b = *p++;
			break;
		case op_markCurPos2:
			break;
		case op_drawStringAtBottom:
			a = *p++;
			break;
		case op_nop:
			break;
		case op_skip3:
			a = *p++;
			b = *p++;
			c = *p++;
			break;
		case op_refreshAll:
			// op_handleKeys
			a = *p++;
			while (a != 0xFF) {
				a = *p++;
			}
			break;
		case op_drawShapeScale:
			a = readWord(p); p += 2;
			if (a & 0x8000) {
				b = (int16_t)readWord(p); p += 2;
				c = (int16_t)readWord(p); p += 2;
			}
			d = readWord(p); p += 2;
			e = *p++;
			f = *p++;
			break;
//		case op_drawShapeScaleRotate:
		case op_drawCreditsText:
			break;
		case op_drawStringAtPos:
			a = readWord(p); p += 2;
			if (a != 0xFFFF) {
				b = *p++;
				c = *p++;
			}
			break;
		case op_handleKeys:
			a = *p++;
			while (a != 0xFF) {
				a = *p++;
			}
			break;
		default:
			fprintf(stderr, "Invalid opcode %d at 0x%04x\n", op, addr);
			break;
		}
		int args[6] = { a, b, c, d, e, f };
		visitOpcode(addr, op, args);
	}
	return 0;
}

static int readFile(const char *path) {
	int count, size = 0;
	FILE *fp;

	fp = fopen(path, "rb");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		assert(size <= MAX_FILESIZE);
		count = fread(_fileBuf, 1, size, fp);
		if (count != size) {
			fprintf(stderr, "Failed to read %d bytes (%d)\n", size, count);
		}
		fclose(fp);
	}
	return size;
}

int main(int argc, char *argv[]) {
	if (argc >= 2) {
		struct stat st;
		if (stat(argv[1], &st) == 0 && S_ISREG(st.st_mode)) {
			int i, count, offset;
			const int size = readFile(argv[1]);
			if (size != 0) {
				count = readWord(_fileBuf) + 1;
				for (i = 0; i < count; ++i) {
					fprintf(_out, "// CINE %d\n", i + 1);
					offset = count * 2;
					if (i != 0) {
						offset += readWord(_fileBuf + (i + 1) * 2);
					}
					visitOpcode = printOpcode;
					parse(_fileBuf + offset, size);
				}
			}
		}
	} else {
		fprintf(stdout, "Usage: %s .cmd .bin .txt\n", argv[0]);
	}
	return 0;
}
