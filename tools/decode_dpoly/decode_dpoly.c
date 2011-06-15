
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

static FILE *fp_CMD;
static FILE *fp_POL;

static uint8_t freadByte(FILE *fp) {
	return fgetc(fp);
}

static uint16_t freadUint16BE(FILE *fp) {
	uint16_t n = fgetc(fp) << 8;
	n |= fgetc(fp);
	return n;
}

static void printPalette(uint16_t num) {
	int i;
	uint16_t offset, color;

	fseek(fp_POL, 0x6, SEEK_SET);
	offset = freadUint16BE(fp_POL);
	fseek(fp_POL, offset + num * 32, SEEK_SET);
	for (i = 0; i < 16; ++i) {
		color = freadUint16BE(fp_POL);
		printf(" 0x%03X", color);
	}
}

static void printShapeVertices() {
	int num;
	int8_t dx, dy;
	int16_t x, y, rx, ry;
	
	num = freadByte(fp_POL);
	if (num & 0x80) {
		x = (int16_t)freadUint16BE(fp_POL);
		y = (int16_t)freadUint16BE(fp_POL);
		rx = freadUint16BE(fp_POL);
		ry = freadUint16BE(fp_POL);
		printf("\n  ellipse ( x=%d,y=%d, rx=%d,ry=%d )", x, y, rx, ry);
	} else if (num == 0) {
		x = (int16_t)freadUint16BE(fp_POL);
		y = (int16_t)freadUint16BE(fp_POL);
		printf("\n  point ( x=%d,y=%d )", x, y);
	} else {
		x = (int16_t)freadUint16BE(fp_POL);
		y = (int16_t)freadUint16BE(fp_POL);
		printf("\n  polygon ( ");
		for (--num; num >= 0; --num) {
			dx = (int8_t)freadByte(fp_POL);
			dy = (int8_t)freadByte(fp_POL);
			printf("x=%d,y=%d ", x + dx, y + dy);
		}
		printf(" )");
	}	
}

static void printShape(uint16_t shapeOffset) {
	int i, count;
	int16_t x, y, alpha, color;
	uint32_t previousOffset;
	uint16_t offset, verticesOffset;
		
	fseek(fp_POL, 0x2, SEEK_SET);
	offset = freadUint16BE(fp_POL);
	fseek(fp_POL, offset + (shapeOffset & 0x7FF) * 2, SEEK_SET);
	shapeOffset = freadUint16BE(fp_POL);

	fseek(fp_POL, 0xE, SEEK_SET);
	offset = freadUint16BE(fp_POL);
	fseek(fp_POL, offset + shapeOffset, SEEK_SET);
	
	count = freadUint16BE(fp_POL);
	for (i = 0; i < count; ++i) {
		verticesOffset = freadUint16BE(fp_POL);
		if (verticesOffset & 0x8000) {
			x = (int16_t)freadUint16BE(fp_POL);
			y = (int16_t)freadUint16BE(fp_POL);
		} else {
			x = 0;
			y = 0;
		}
		alpha = (verticesOffset & 0x4000) != 0 ? 1 : 0;
		color = freadByte(fp_POL);
		
		previousOffset = ftell(fp_POL);
		
		fseek(fp_POL, 0xA, SEEK_SET);
		offset = freadUint16BE(fp_POL);
		fseek(fp_POL, offset + (verticesOffset & 0x3FFF) * 2, SEEK_SET);
		verticesOffset = freadUint16BE(fp_POL);

		fseek(fp_POL, 0x12, SEEK_SET);
		offset = freadUint16BE(fp_POL);
		fseek(fp_POL, offset + verticesOffset, SEEK_SET);

		printf("\n  shape %d/%d x %d y %d a %d c %d", i, count, x, y, alpha, color);
		printShapeVertices();

		fseek(fp_POL, previousOffset, SEEK_SET);
	}
}

static void decodeDPoly(FILE *fp, const char *sequenceName, int num) {
	uint8_t op;
	uint32_t addr;
	int offset1, offset2;

	if (num != 0) {
		fseek(fp, (num + 1) * 2, SEEK_SET);
		offset2 = freadUint16BE(fp);
	} else {
		offset2 = 0;
	}
	fseek(fp, 0, SEEK_SET);
	offset1 = (freadUint16BE(fp) + 1) * 2;
	
	fseek(fp, offset1 + offset2, SEEK_SET);

	printf("// sequenceName '%s' offsets 0x%X/0x%X\n", sequenceName, offset1, offset2);

	while (1) {
		op = freadByte(fp);
		if (op & 0x80) {
			break;
		}
		op >>= 2;
		addr = ftell(fp);
		printf("[%04X] (%02X) ", addr, op);
		switch (op) {
		case 0: {
				printf("MARK_CURRENT_POS");
			}
			break;
		case 1: {
				uint8_t clr = freadByte(fp);
				printf("REFRESH_SCREEN ( clear_screen=%d )", clr);
			}
			break;
		case 2: {
				uint8_t delay = freadByte(fp);
				printf("WAIT_FOR_SYNC ( delay=%d )", delay);
			}
			break;
		case 3: {
				int16_t x, y;
				uint16_t shapeOffset = freadUint16BE(fp);
				if (shapeOffset & 0x8000) {
					x = freadUint16BE(fp);
					y = freadUint16BE(fp);
				} else {
					x = 0;
					y = 0;
				}
				printf("DRAW_SHAPE ( offset=0x%X, x=%d, y=%d )", shapeOffset & 0x7FFF, x, y);
				printShape(shapeOffset);
			}
			break;
		case 4: {
				uint8_t spal = freadByte(fp);
				uint8_t dpal = freadByte(fp);
				printf("SET_PALETTE ( spal=%d, dpal=%d )", spal, dpal);
				printPalette(spal);
			}
			break;
		case 5: {
				printf("MARK_CURRENT_POS");
			}
			break;
		case 6: {
				uint16_t str = freadUint16BE(fp);
				printf("DRAW_STRING_AT_BOTTOM ( str=%d )", str);
			}
			break;
		case 7: {
				printf("NOP");
			}
			break;
		case 8: {
				freadByte(fp);
				freadUint16BE(fp);
				printf("SKIP3\n");
			}
			break;
		case 9: {
				printf("REFRESH_ALL");
			}
			break;
		case 10: {
				int16_t x, y, z, ix, iy;
				uint16_t shapeOffset = freadUint16BE(fp);
				if (shapeOffset & 0x8000) {
					x = freadUint16BE(fp);
					y = freadUint16BE(fp);
				} else {
					x = 0;
					y = 0;
				}
				z = 512 + freadUint16BE(fp);
				ix = freadUint16BE(fp);
				iy = freadUint16BE(fp);
				printf("DRAW_SHAPE_SCALE ( offset=0x%X, x=%d, y=%d, z=%d, ix=%d, iy=%d )", shapeOffset & 0x7FFF, x, y, z, ix, iy);
				printShape(shapeOffset);
			}
			break;
		case 11: {
				int16_t x, y, z, ix, iy, r1, r2, r3;
				uint16_t shapeOffset = freadUint16BE(fp);
				if (shapeOffset & 0x8000) {
					x = freadUint16BE(fp);
					y = freadUint16BE(fp);
				} else {
					x = 0;
					y = 0;
				}
				z = 512;
				if (shapeOffset & 0x4000) {
					z += freadUint16BE(fp);
				}
				ix = freadUint16BE(fp);
				iy = freadUint16BE(fp);
				r1 = freadUint16BE(fp);
				if (shapeOffset & 0x2000) {
					r2 = freadUint16BE(fp);
				} else {
					r2 = 180;
				}
				if (shapeOffset & 0x1000) {
					r3 = freadUint16BE(fp);
				} else {
					r3 = 90;
				}
				printf("DRAW_SHAPE_SCALE_ROTATE ( offset=0x%X, x=%d, y=%d, z=%d, ix=%d, iy=%d, r1=%d, r2=%d, r3=%d )", shapeOffset & 0x7FFF, x, y, z, ix, iy, r1, r2, r3);
				printShape(shapeOffset);
			}
			break;
		case 12: {
				printf("DRAW_CREDITS_TEXT");
			}
			break;
		case 13: {
				int16_t x, y;
				uint16_t str = freadUint16BE(fp);
				if (str != 0xFFFF) {
					x =  freadUint16BE(fp) * 8;
					y =  freadUint16BE(fp) * 8;
				} else {
					x = 0;
					y = 0;
				}
				printf("DRAW_STRING_AT_POS ( str=%d, x=%d, y=%d )", str, x, y);
			}
			break;
		default:
			printf("Invalid opcode %d\n", op);
			exit(0);
			break;
		}
		printf("\n");
	}
}

int main(int argc, char *argv[]) {
	if (argc == 4) {
		fp_CMD = fopen(argv[1], "rb");
		fp_POL = fopen(argv[2], "rb");
		if (fp_CMD && fp_POL) {
			decodeDPoly(fp_CMD, argv[1], atoi(argv[3]));
		}
	}
	return 0;
}
