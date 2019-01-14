
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "bitmap.h"
#include "decode_3docel.h"
#include "decode_3dostr.h"
#include "endian.h"
#include "fileio.h"
#include "unpack.h"

static uint8_t _bitmapBuffer[320 * 200 * sizeof(uint32_t)];

static void decodeCel(FILE *fp, const char *fname) {
	CelPicture cp;
	if (decode_3docel(fp, &cp) == 0) {
		if (cp.bpp == 6 || cp.bpp == 8) {
			uint8_t palette[256 * 3];
			for (int i = 0; i < 256; ++i) {
				const uint8_t r = ((cp.pal[i] >> 10) & 31) << 3;
				const uint8_t g = ((cp.pal[i] >>  5) & 31) << 3;
				const uint8_t b =  (cp.pal[i]        & 31) << 3;
				palette[i * 3]     = r;
				palette[i * 3 + 1] = g;
				palette[i * 3 + 2] = b;
			}
			saveBMP(fname, cp.data, palette, cp.w, cp.h);
		} else if (cp.bpp == 16) {
			saveTGA(fname, cp.data, cp.w, cp.h);
		} else {
			fprintf(stderr, "Unhandled bpp %d\n", cp.bpp);
		}
	}
}

static void decodeAnim(FILE *fp) {
	char tag[4];
	const uint32_t size = freadTag(fp, tag);
	assert(memcmp(tag, "ANIM", 4) == 0);
	uint32_t version = freadUint32BE(fp);
	uint32_t type = freadUint32BE(fp);
	uint32_t frames = freadUint32BE(fp);
	uint32_t rate = freadUint32BE(fp);
	fprintf(stdout, "anim version %d type %d frames %d rate %d\n", version, type, frames, rate);
	fseek(fp, size, SEEK_SET);
	// TODO: 'PLUT' can be common to all frames...
	for (int i = 0; i < frames; ++i) {
		char name[16];

		fprintf(stdout, "frame #%d at 0x%lx\n", i, ftell(fp));
		snprintf(name, sizeof(name), "anim_%03d.tga", i);
		decodeCel(fp, name);
	}
}

static void decodeFontChr(FILE *fp) {
	uint8_t buf[8 * 4];
	int chr = 32;
	while (1) {
		fread(buf, 1, sizeof(buf), fp);
		if (feof(fp)) {
			break;
		}
		int histogram = 0;
		fprintf(stdout, "Character %d '%c'\n", chr, chr);
		for (int y = 0; y < 8; ++y) {
			for (int x = 0; x < 4; ++x) {
				const int c = buf[y * 4 + x];
				fprintf(stdout, "%02x", c);
				if (c != 0) {
					++histogram;
				}
			}
			fprintf(stdout, "\n");
		}
		fprintf(stdout, "Character 0x%02x histogram %d\n", chr, histogram);
		++chr;
	}
}

// 16x16x4bpp
static void decodeIcons(FILE *fp) {
	fseek(fp, 0, SEEK_END);
	const uint32_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	static const int ICON_W = 16;
	static const int ICON_H = 16;
	// icons are 16x16 4bpp
	assert(size % (16 * 8) == 0);
	const int count = size / (16 * 8);
	const int pitch = count * ICON_W;

	uint8_t buf[16 * 8];
	for (int i = 0; i < count; ++i) {
		fread(buf, 1, sizeof(buf), fp);
		const int offset = i * ICON_W;
		for (int y = 0; y < ICON_H; ++y) {
			for (int x = 0; x < ICON_W; x += 2) {
				const uint8_t color = buf[y * 8 + x / 2];
				_bitmapBuffer[y * pitch + offset + x]     = color >> 4;
				_bitmapBuffer[y * pitch + offset + x + 1] = color & 15;
			}
		}
	}
	uint8_t paletteBuffer[256 * 3];
	for (int i = 0; i < 16; ++i) {
		const uint8_t color = i * 16;
		paletteBuffer[3 * i] = paletteBuffer[3 * i + 1] = paletteBuffer[3 * i + 2] = color;
	}
	saveBMP("icons.3do.bmp", _bitmapBuffer, paletteBuffer, count * ICON_W, ICON_H);
}

//
// .TBN
//

#define MAX_TEXTS 54
#define MAX_TEXT_LENGTH 256

#define MAX_OFFSETS 200

static uint8_t _textsTable[MAX_TEXTS][MAX_TEXT_LENGTH + 1];
static uint16_t _tbnOffsets[MAX_OFFSETS];

static const char *_tbnNames[] = {
	"LEVEL1.TBN",
	"LEVEL2.TBN",
	"LEVEL3.TBN",
	"LEVEL4_1.TBN",
	"LEVEL4_2.TBN",
	"LEVEL5_1.TBN",
	"LEVEL5_2.TBN",
	0
};

static const int _tbnFirstOffsets[] = {
	0x2E, 0x4C, 0x10, 0x1A, 0x18, 0x22, 0x22, 0
};

static void decodeTextBin(FILE *fp) {
	uint16_t prev = 0;
	for (int i = 0; i < 0x168; i += 2) {
		uint16_t offset = freadUint16BE(fp);
		fprintf(stdout, "index %d 0x%x %d difference %d\n", i / 2, offset, offset, offset - prev);
		prev = offset;
		_tbnOffsets[i / 2] = offset;
	}
	// data start at index[7] + contains _stringsTable + levels .TBN
	int tbn = 0;
	int num = 0;
	char buf[1024];
	int len = 0;
	while (1) {
		char chr = fgetc(fp);
		if (feof(fp)) {
			break;
		}
		if (tbn == 0 && num < MAX_TEXTS && chr < MAX_TEXT_LENGTH) {
			_textsTable[num][len] = chr;
		}
		buf[len++] = (chr != 0 && chr < 32) ? 32 : chr;
		if (chr == 0) {
			fprintf(stdout, "tbn #%d str #%d len %d '%s'\n", tbn, num, len, buf);
			++num;
			if (len == 1) {
				++tbn;
				num = 0;
			}
			len = 0;
		}
	}
	static const int GAME_STRINGS_COUNT = 0x5A / 2;
	for (int i = 0; i < 7; ++i) {
		uint16_t tbnOffset = _tbnOffsets[i];
		tbnOffset += _tbnOffsets[tbnOffset / 2];
		fprintf(stdout, "%s starts at 0x%x\n", _tbnNames[i], tbnOffset);
	}
	for (int i = 0; i < GAME_STRINGS_COUNT; ++i) {
		uint16_t gameStringOffset = _tbnOffsets[7 + i];
		fprintf(stdout, "gameString #%d at 0x%x\n", i, gameStringOffset);
	}

	// _stringsTableJP
	const int startOffset = _tbnOffsets[7];
	const int endOffset = _tbnOffsets[0] + _tbnOffsets[_tbnOffsets[0] / 2]; // beginning of LEVEL1.TBN

	// make text offsets relative to the beginning of the buffer
	uint16_t textOffsets[GAME_STRINGS_COUNT];
	uint16_t baseOffset = GAME_STRINGS_COUNT * sizeof(uint16_t);
	for (int i = 0; i < GAME_STRINGS_COUNT; ++i) {
		textOffsets[i] = baseOffset + (_tbnOffsets[7 + i] - _tbnOffsets[7]);
	}
	FILE *out = fopen("STRINGS_JP.BIN", "wb");
	if (out) {
		// LE offsets
		for (int i = 0; i < GAME_STRINGS_COUNT; ++i) {
			fputc(textOffsets[i] & 255, out);
			fputc(textOffsets[i] >> 8, out);
		}
		// data
		fseek(fp, startOffset, SEEK_SET);
		while (ftell(fp) != endOffset) {
			fputc(fgetc(fp), out);
		}
		fclose(out);
	}

	// .tbn
	for (int i = 0; _tbnNames[i]; ++i) {
		int tbnStartOffset = _tbnOffsets[i];
		assert((tbnStartOffset & 1) == 0);
		tbnStartOffset /= 2;

		// make text offsets relative to the beginning of the buffer
		uint16_t offsets[256];
		for (int j = 0; j < _tbnFirstOffsets[i] / 2; ++j) {
			offsets[j] = _tbnFirstOffsets[i] + _tbnOffsets[tbnStartOffset + j] - _tbnOffsets[tbnStartOffset];
		}

		out = fopen(_tbnNames[i], "wb");
		if (out) {
			// LE offsets
			for (int j = 0; j < _tbnFirstOffsets[i] / 2; ++j) {
				fputc(offsets[j] & 255, out);
				fputc(offsets[j] >> 8, out);
			}
			// data '\0' terminated strings
			for (int j = 0; j < _tbnFirstOffsets[i] / 2; ++j) {
				fseek(fp, tbnStartOffset * 2 + _tbnOffsets[tbnStartOffset + j], SEEK_SET);
				while (1) {
					const char chr = fgetc(fp);
					fputc(chr, out);
					if (!chr) {
						break;
					}
				}
			}
			fclose(out);
		}
	}
}

//
// .CHR
//

static uint8_t _fontChr[12800];
static int _fontLen;

#define HORIZONTAL_MARGIN 0
#define VERTICAL_MARGIN 2

static uint8_t _textBitmap[1024 * MAX_TEXTS * (8 + VERTICAL_MARGIN)];
static uint8_t _textPalette[256 * 3];

static void drawChar(int x, int y, int chr, int offset) {
	uint8_t *p = _textBitmap + y * 1024 + x;
	uint8_t *f = _fontChr;

	f += chr * 8 * 4 + offset;

	for (int j = 0; j < 8; ++j) {
		for (int i = 0; i < 4; ++i) {
			const int c = f[j * 4 + i];
			if ((c >> 4) != 0) {
				p[2 * i] = c >> 4;
			}
			if ((c & 15) != 0) {
				p[2 * i + 1] = c & 15;
			}
		}
		p += 1024;
	}
}

static void drawString(int x, int y, const uint8_t *str) {
	while (*str && x < 1024) {
		if (*str >= 32 && *str < 32 + 200) { // 200 == 6400 / (8 * 4)
			drawChar(x, y, *str - 32, 0);
			if (_fontLen == 12800) {
				drawChar(x, y + 8, *str - 32, 6400);
			}
			if (*str >= 0xD1) {
				fprintf(stderr, "Empty bitmap for character 0x%x pos %d,%d\n", *str, x, y);
			}
		} else {
			// fprintf(stderr, "Unhandled character 0x%02x\n", *str);
		}
		x += 8 + HORIZONTAL_MARGIN;
		++str;
	}
}

static const uint8_t _textPal[] = {
	0x00, 0x00, 0x11, 0x01, 0x22, 0x02, 0xEF, 0x0E, 0x00, 0x0F, 0xF0, 0x0F, 0xA0, 0x0E, 0xB0, 0x0F,
	0xA0, 0x0E, 0xA0, 0x0E, 0xAA, 0x0A, 0xF0, 0x00, 0xCC, 0x0C, 0xDF, 0x0D, 0xEE, 0x0E, 0xEE, 0x0E
};

//static const char *kFontChr = "FONT.CHR";
static const char *kFontChr = "FONT8JAP.CHR";

static void renderText() {
	FILE *fp = fopen(kFontChr, "rb");
	if (fp) {
		_fontLen = fread(_fontChr, 1, sizeof(_fontChr), fp);
		fclose(fp);
	}
	for (int i = 0; i < MAX_TEXTS; ++i) {
		drawString(0, i * (8 + VERTICAL_MARGIN), _textsTable[i]);
	}
	_textPalette[0] = 0x44;
	_textPalette[1] = 0x88;
	_textPalette[2] = 0x88;

	_textPalette[3] = 0xFF;
	_textPalette[4] = 0xBB;
	_textPalette[5] = 0x00;

	_textPalette[6] = 0x00;
	_textPalette[7] = 0x00;
	_textPalette[8] = 0x00;

	saveBMP("text.bmp", _textBitmap, _textPalette, 1024, MAX_TEXTS * (8 + VERTICAL_MARGIN));
}

//
// .PAL
//

static void convertAmigaColor(uint16_t color, uint8_t *rgb) {
	rgb[0] = (color >> 8) & 15;
	rgb[0] = (rgb[0] << 4) | rgb[0];
	rgb[1] = (color >> 4) & 15;
	rgb[1] = (rgb[1] << 4) | rgb[1];
	rgb[2] =  color       & 15;
	rgb[2] = (rgb[2] << 4) | rgb[2];
}

static void fillRect(uint8_t *dst, uint32_t pitch, int x, int y, int w, int h, uint8_t color) {
	const int y2 = y + h;
	for (; y < y2; ++y) {
		memset(dst + y * pitch + x, color, w);
	}
}

static void decodeLevelPal(FILE *fp, const char *name) {
	static const int W = 16;
	static const int H = 16;

	fseek(fp, 0, SEEK_END);
	const int fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// 16 colors on uint16_t (0rgb)
	assert((fileSize % 32) == 0);
	const int count = fileSize / 32;

	// check for less than 16 palettes, so we can output to 256 colors paletted
	assert(count <= 16);

	uint8_t paletteBuffer[256 * 3];
	for (int i = 0; i < count; ++i) {
		const uint8_t base = i * 16;
		for (int j = 0; j < 16; ++j) {
			const uint8_t color = base + j;
			fillRect(_bitmapBuffer, W * 16, j * W, i * H, W, H, color);
			convertAmigaColor(freadUint16BE(fp), paletteBuffer + color * 3);
		}
	}
	saveBMP(name, _bitmapBuffer, paletteBuffer, W * 16, H * count);
}

static const struct {
	const char *ext;
	void (*decode)(const char *name, FILE *fp);
} _decoders[] {
	{ 0, 0 }
}; // keep 'ext' alphabetically sorted, bsearch

int main(int argc, char *argv[]) {
	if (argc == 3) {
		FILE *fp = fopen(argv[2], "rb");
		if (fp) {
			if (strcmp(argv[1], "-anim") == 0) {
				decodeAnim(fp);
			} else if (strcmp(argv[1], "-cel") == 0) {
				const char *p = strrchr(argv[2], '/');
				if (!p) {
					p = argv[2];
				} else {
					++p;
				}
				char *name = (char *)malloc(strlen(p) + 4 /* '.tga' */ + 1);
				if (name) {
					strcpy(name, p);
					strcat(name, ".tga");
					decodeCel(fp, name);
					free(name);
				}
			}
		}
	}
	if (argc == 2) {
		FILE *fp = fopen(argv[1], "rb");
		if (fp) {
			const char *ext = strrchr(argv[1], '.');
			if (ext) {
				if (strcmp(ext, ".3DO") == 0) {
					decodeIcons(fp);
				} else if (strcmp(ext, ".BIN") == 0) {
					decodeTextBin(fp);
					renderText();
				} else if (strcmp(ext, ".CHR") == 0) {
					decodeFontChr(fp);
				} else if (strcasecmp(ext, ".CPC") == 0) {
					decode_3dostr(fp);
				} else if (strcasecmp(ext, ".SUB") == 0) {
					// 6bpp - japanese subtitles
					int frames = 0;
					do {
						char name[16];
						snprintf(name, sizeof(name), "sub%03d.tga", frames);
						decodeCel(fp, name);
						++frames;
					} while (!feof(fp));
					fprintf(stdout, "Decoded %d frames\n", frames);
				} else if (strcasecmp(ext, ".CT") == 0 || strcasecmp(ext, ".CT2") == 0) {
					static uint8_t buffer[0x1D00];
					const int size = fread(buffer, 1, sizeof(buffer), fp);
					int ret = bytekiller_unpack(_bitmapBuffer, sizeof(_bitmapBuffer), buffer, size);
					fprintf(stdout, "Unpacked %s %d\n", ext, ret);
				} else if (strcasecmp(ext, ".LEV") == 0 || strcasecmp(ext, ".LV2") == 0) {
					uint32_t offsets[64];
					for (int i = 0; i < 64; ++i) {
						offsets[i] = freadUint32BE(fp);
					}
					static uint8_t buffer[82216];
					fread(buffer, 1, sizeof(buffer), fp);
					uint32_t prevOffset = 64 * sizeof(uint32_t);
					for (int i = 0; i < 64; ++i) {
						const uint32_t size = offsets[i] - prevOffset;
						if (size != 0) {
							const uint32_t relativeOffset = offsets[i] - 64 * sizeof(uint32_t);
							int ret = bytekiller_unpack(_bitmapBuffer, sizeof(_bitmapBuffer), buffer, relativeOffset);
							fprintf(stdout, "Unpacked %s room %d ret %d\n", ext, i, ret);
						}
						offsets[i] = prevOffset;
					}
				} else if (strcasecmp(ext, ".MBK") == 0) {
#define MAX_MBK 128
					struct {
						uint32_t offset;
						int16_t len32;
					} mbk[MAX_MBK];
					fseek(fp, 0, SEEK_END);
					const uint32_t fileSize = ftell(fp);
					fseek(fp, 0, SEEK_SET);
					int count = 0;
					for (int i = 0; i < MAX_MBK; ++i) {
						const uint32_t offset = freadUint32BE(fp);
						if (offset >= fileSize) {
							break;
						}
						assert(i < MAX_MBK);
						mbk[i].offset = offset;
						mbk[i].len32 = freadUint16BE(fp);
						++count;
					}
					for (int i = 0; i < count; ++i) {
						const bool compressed = mbk[i].len32 > 0;
						int len;
						if (mbk[i].len32 < 0) {
							len = (-mbk[i].len32) * 32;
						} else {
							fseek(fp, mbk[i].offset - 4, SEEK_SET);
							// bytekiller compressed data
							len = freadUint32BE(fp);
						}
						fprintf(stdout, "MBK #%d offset 0x%x len %d (compressed %d)\n", i, mbk[i].offset, len, compressed);
					}
				} else if (strcasecmp(ext, ".PAL") == 0) {
					const char *p = strrchr(argv[1], '/');
					if (!p) {
						p = argv[1];
					} else {
						++p;
					}
					char *name = (char *)malloc(strlen(p) + 4 /* '.bmp' */ + 1);
					if (name) {
						strcpy(name, p);
						strcat(name, ".bmp");
						decodeLevelPal(fp, name);
						free(name);
					}
				}
			}
			fclose(fp);
		}
	}
	return 0;
}
