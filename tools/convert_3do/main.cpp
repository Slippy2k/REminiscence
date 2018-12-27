
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cinepak.h"
extern "C" {
#include "tga.h"
}

static uint8_t _bitmapBuffer[320 * 200 * sizeof(uint32_t)];
static uint8_t _rgbBuffer[320 * 200 * sizeof(uint32_t)];

static uint8_t clip8(int a) {
	if (a < 0) {
		return 0;
	} else if (a > 255) {
		return 255;
	} else {
		return a;
	}
}

static uint16_t yuv_to_rgb565(int y, int u, int v) {
	int r = int(y + (1.370705 * (v - 128)));
	r = clip8(r) >> 3;
	int g = int(y - (0.698001 * (v - 128)) - (0.337633 * (u - 128)));
	g = clip8(g) >> 2;
	int b = int(y + (1.732446 * (u - 128)));
	b = clip8(b) >> 3;
	return (r << 11) | (g << 5) | b;
}

static void uyvy_to_rgb565(const uint8_t *in, int len, uint16_t *out) {
	assert((len & 3) == 0);
	for (int i = 0; i < len; i += 4, in += 4) {
		const int u  = in[0];
		const int y0 = in[1];
		const int v  = in[2];
		const int y1 = in[3];
		*out++ = yuv_to_rgb565(y0, u, v);
		*out++ = yuv_to_rgb565(y1, u, v);
	}
}

static uint16_t rgb555_to_565(uint16_t color) {
	const int r = (color >> 10) & 31;
	const int g = (color >>  5) & 31;
	const int b =  color        & 31;
	return (r << 11) | (g << 6) | b;
}

struct OutputBuffer {
	void setup(int w, int h, CinepakDecoder *decoder) {
		_bufSize = w * h * 2;
		_buf = (uint8_t *)malloc(_bufSize);
		decoder->_yuvFrame = _buf;
		decoder->_yuvW = w;
		decoder->_yuvH = h;
		decoder->_yuvPitch = w * 2;
		_w = w;
		_h = h;
	}
	void dump(int num) {
		if (1) {
			char filename[64];
			snprintf(filename, sizeof(filename), "out/%04d.tga", num);
			uyvy_to_rgb565(_buf, _bufSize, (uint16_t *)_rgbBuffer);
			struct TgaFile *tga = tgaOpen(filename, _w, _h, 16);
			if (tga) {
				tgaWritePixelsData(tga, _rgbBuffer, _bufSize);
				tgaClose(tga);
			}
			return;
		}
		char name[16];
		snprintf(name, sizeof(name), "out/%04d.uyvy", num);
		FILE *fp = fopen(name, "wb");
		if (fp) {
			fwrite(_buf, _bufSize, 1, fp);
			fclose(fp);
			char cmd[256];
			snprintf(cmd, sizeof(cmd), "convert -size 256x128 -depth 8 out/%04d.uyvy out/%04d.png", num, num);
			system(cmd);
		}
	}

	uint8_t *_buf;
	uint32_t _bufSize;
	int _w, _h;
};

static uint16_t freadUint16LE(FILE *fp) {
	uint8_t buf[2];
	fread(buf, 1, sizeof(buf), fp);
	return buf[0] | (buf[1] << 8);
}

static uint16_t freadUint16BE(FILE *fp) {
	uint8_t buf[2];
	fread(buf, 1, sizeof(buf), fp);
	return (buf[0] << 8) | buf[1];
}

static uint32_t freadUint32BE(FILE *fp) {
	uint8_t buf[4];
	fread(buf, 1, sizeof(buf), fp);
	return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

static uint32_t readTag(FILE *fp, char *type) {
	fread(type, 4, 1, fp);
	uint32_t size = freadUint32BE(fp);
	return size;
}

struct ccb_t {
	uint32_t version;
	uint32_t flags;
	uint16_t ppmp0;
	uint16_t ppmp1;
	uint32_t pre0;
	uint32_t pre1;
	uint32_t width;
	uint32_t height;
};

struct anim_t {
	uint32_t version;
	uint32_t type;
	uint32_t frames;
	uint32_t rate;
};

static const uint8_t _cel_bitsPerPixelLookupTable[8] = {
        0, 1, 2, 4, 6, 8, 16, 0
};

static uint16_t _cel_bitsMask[17] = {
        0,
        0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F, 0x007F, 0x00FF,
        0x01FF, 0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF
};

// BitReader
static int _celBits_bits;
static int _celBits_size;

static void decodeCel_reset(FILE *fp) {
	_celBits_bits = fgetc(fp);
	_celBits_size = 8;
}

static uint32_t decodeCel_readBits(FILE *fp, int count) {
	uint32_t value = 0;
	while (count != 0) {
		if (count < _celBits_size) {
			value |= (_celBits_bits >> (_celBits_size -  count)) & _cel_bitsMask[count];
			_celBits_size -= count;
			count = 0;
		} else {
			count -= _celBits_size;
			value |= (_celBits_bits & _cel_bitsMask[_celBits_size]) << count;
			// refill
			_celBits_bits = fgetc(fp);
			_celBits_size = 8;
		}
	}
	return value;
}

static void decodeCel_PDAT(const struct ccb_t *ccb, FILE *fp, uint32_t size) {
	const int ccbPRE0_bitsPerPixel = _cel_bitsPerPixelLookupTable[ccb->pre0 & 7];
	const int bpp = (ccbPRE0_bitsPerPixel < 8) ? 8 : ccbPRE0_bitsPerPixel;
	assert(ccb->width * ccb->height * bpp / 8 <= sizeof(_bitmapBuffer));
	if (ccb->flags & 0x200) { // RLE
		for (int j = 0; j < ccb->height; ++j) {
			uint8_t *dst = _bitmapBuffer + j * ccb->width * bpp / 8;
			const int pos = ftell(fp);
			int lineSize = (ccbPRE0_bitsPerPixel >= 8) ? freadUint16BE(fp) : fgetc(fp);
			decodeCel_reset(fp);
			int w = ccb->width;
			while (w > 0) {
				int type = decodeCel_readBits(fp, 2);
				int count = decodeCel_readBits(fp, 6) + 1;
				// fprintf(stdout, "\t type %d count %d\n", type, count);
				if (type == 0) { // PACK_EOL
					break;
				}
				if (w - count < 0) {
					count = w;
				}
				switch (type) {
				case 1: // PACK_LITERAL
					for (int i = 0; i < count; ++i) {
						if (bpp == 16) {
							const uint16_t color = decodeCel_readBits(fp, ccbPRE0_bitsPerPixel);
							*(uint16_t *)dst = color;
							dst += 2;
						} else {
							const uint8_t color = decodeCel_readBits(fp, ccbPRE0_bitsPerPixel);
							*dst++ = color;
						}
					}
					break;
				case 2: // PACK_TRANSPARENT
					dst += count * bpp / 8;
					break;
				case 3: // PACK_REPEAT
					if (bpp == 16) {
						const uint16_t color = decodeCel_readBits(fp, ccbPRE0_bitsPerPixel);
						for (int i = 0; i < count; ++i) {
							*(uint16_t *)dst = color;
							dst += 2;
						}
					} else {
						const uint8_t color = decodeCel_readBits(fp, ccbPRE0_bitsPerPixel);
						memset(dst, color, count);
						dst += count;
					}
					break;
				}
				w -= count;
			}
			const int align = (lineSize + 2) * sizeof(uint32_t);
			// fprintf(stdout, "y %d count w %d lineSize %d (%d) offset %d\n", j, w, lineSize, align, int(ftell(fp) - pos));
			fseek(fp, pos + align, SEEK_SET);
		}
	} else {
		fread(_bitmapBuffer, 1, ccb->height * ccb->width * bpp / 8, fp);
	}
}

static ccb_t ccb;
static int plutSize;

static void decodeCel(FILE *fp, const char *fname) {
	uint16_t rgb555[256];
	while (1) {
		const uint32_t pos = ftell(fp);
		char tag[5];
		const uint32_t size = readTag(fp, tag);
		if (feof(fp)) {
			break;
		}
		tag[4] = 0;
		fprintf(stdout, "Found tag '%s' size %d\n", tag, size);
		if (memcmp(tag, "CCB ", 4) == 0) {
			ccb.version = freadUint32BE(fp);
			ccb.flags = freadUint32BE(fp);
			fseek(fp, 3 * 4, SEEK_CUR);
			uint32_t xpos = freadUint32BE(fp);
			uint32_t ypos = freadUint32BE(fp);
			uint32_t hdx = freadUint32BE(fp);
			uint32_t hdy = freadUint32BE(fp);
			uint32_t vdx = freadUint32BE(fp);
			uint32_t vdy = freadUint32BE(fp);
			uint32_t hddx = freadUint32BE(fp);
			uint32_t hddy = freadUint32BE(fp);
			fprintf(stdout, "pos 0x%x,0x%x hd 0x%x,0x%x vd 0x%x,0x%x, hdd 0x%x,0x%x\n", xpos, ypos, hdx, hdy, vdx, vdy, hddx, hddy);
			uint32_t ppmp = freadUint32BE(fp);
			ccb.ppmp0 = ppmp >> 16;
			ccb.ppmp1 = ppmp & 0xFFFF;
			ccb.pre0 = freadUint32BE(fp);
			ccb.pre1 = freadUint32BE(fp);
			ccb.width = freadUint32BE(fp);
			ccb.height = freadUint32BE(fp);
			fprintf(stdout, "version %d flags 0x%x width %d height %d\n", ccb.version, ccb.flags, ccb.width, ccb.height);

			// bit9 - 0x200 - compressed
			// bit5 - RGB(0,0,0) is actual black not transparent

			int ccbPRE0_bitsPerPixel = _cel_bitsPerPixelLookupTable[ccb.pre0 & 7];
			int ccbPRE0_height = ((ccb.pre0 >> 6) & 0x3FF) + 1;
			int ccbPRE1_width  = (ccb.pre1 & 0x3FF) + 1;

			fprintf(stdout, "ccbPRE bits %d width %d height %d rle %d\n", ccbPRE0_bitsPerPixel, ccbPRE1_width, ccbPRE0_height, (ccb.flags & 0x200) != 0);

		} else if (memcmp(tag, "PLUT", 4) == 0) {
			int count = freadUint32BE(fp);
			fprintf(stdout, "PLUT count %d\n", count);
			assert(count <= 256);
			memset(rgb555, 0, sizeof(rgb555));
			for (int i = 0; i < count; ++i) {
				rgb555[i] = freadUint16BE(fp);
			}
			plutSize = count;
		} else if (memcmp(tag, "PDAT", 4) == 0) {
			decodeCel_PDAT(&ccb, fp, size);
			// stop on first PDAT
			break;
		} else {
			fprintf(stderr, "Unhandled tag '%c%c%c%c' size %d\n", tag[0], tag[1], tag[2], tag[3], size);
		}
		fseek(fp, pos + size, SEEK_SET);
	}
	int bpp = _cel_bitsPerPixelLookupTable[ccb.pre0 & 7];
	fprintf(stdout, "tga width %d height %d bpp %d\n", ccb.width, ccb.height, bpp);
	struct TgaFile *tga = tgaOpen(fname, ccb.width, ccb.height, bpp);
	if (tga) {
		if (bpp == 8) {
			assert(plutSize != 0);
			uint8_t palette[256 * 3];
			if (0) {
				const int colorsCount = 1 << _cel_bitsPerPixelLookupTable[ccb.pre0 & 7];
				for (int i = 0; i < colorsCount; ++i) {
					uint8_t color = i * 255 / colorsCount;
					palette[i * 3]     = color;
					palette[i * 3 + 1] = color;
					palette[i * 3 + 2] = color;
				}
			} else {
				for (int i = 0; i < 256; ++i) {
					const uint8_t r = ((rgb555[i % plutSize] >> 10) & 31) << 3;
					const uint8_t g = ((rgb555[i % plutSize] >>  5) & 31) << 3;
					const uint8_t b =  (rgb555[i % plutSize]        & 31) << 3;
					palette[i * 3]     = r;
					palette[i * 3 + 1] = g;
					palette[i * 3 + 2] = b;
				}
			}
			tgaSetLookupColorTable(tga, palette);
			tgaWritePixelsData(tga, _bitmapBuffer, ccb.width * ccb.height * bpp / 8);
		} else if (bpp == 16) {
			const int len = ccb.width * ccb.height * sizeof(uint16_t);
			tgaWritePixelsData(tga, _bitmapBuffer, len);
		} else {
			fprintf(stderr, "Unhandled bpp %d\n", bpp);
		}
		tgaClose(tga);
	}
}

static void decodeAnim(FILE *fp) {
	char tag[4];
	const uint32_t size = readTag(fp, tag);
	assert(memcmp(tag, "ANIM", 4) == 0);
	anim_t anim;
	anim.version = freadUint32BE(fp);
	anim.type = freadUint32BE(fp);
	anim.frames = freadUint32BE(fp);
	anim.rate = freadUint32BE(fp);
	fprintf(stdout, "anim version %d type %d frames %d rate %d\n", anim.version, anim.type, anim.frames, anim.rate);
	fseek(fp, size, SEEK_SET);
	for (int i = 0; i < anim.frames; ++i) {
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

static uint8_t _fontChr[6400];

#define HORIZONTAL_MARGIN 0
#define VERTICAL_MARGIN 2

static uint8_t _textBitmap[1024 * MAX_TEXTS * (8 + VERTICAL_MARGIN)];
static uint8_t _textPalette[256 * 3];

static void drawChar(int x, int y, int chr) {
	uint8_t *p = _textBitmap + y * 1024 + x;
	uint8_t *f = &_fontChr[chr * 8 * 4];

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
			drawChar(x, y, *str - 32);
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

static void renderTextTga() {
	FILE *fp = fopen("FONT8JAP.CHR", "rb");
	if (fp) {
		fread(_fontChr, 1, sizeof(_fontChr), fp);
		fclose(fp);
	}
	for (int i = 0; i < MAX_TEXTS; ++i) {
		drawString(0, i * (8 + VERTICAL_MARGIN), _textsTable[i]);
	}
	struct TgaFile *tga = tgaOpen("test.tga", 1024, MAX_TEXTS * (8 + VERTICAL_MARGIN), 8);
	if (tga) {
		_textPalette[0] = 0x44;
		_textPalette[1] = 0x88;
		_textPalette[2] = 0x88;

		_textPalette[3] = 0xFF;
		_textPalette[4] = 0xBB;
		_textPalette[5] = 0x00;

		_textPalette[6] = 0x00;
		_textPalette[7] = 0x00;
		_textPalette[8] = 0x00;

		tgaSetLookupColorTable(tga, _textPalette);
		tgaWritePixelsData(tga, _textBitmap, sizeof(_textBitmap));

		tgaClose(tga);
	}
}

int main(int argc, char *argv[]) {
	if (argc == 2) {
		FILE *fp = fopen(argv[1], "rb");
		if (fp) {
			const char *ext = strrchr(argv[1], '.');
			if (ext) {
				if (strcmp(ext, ".CHR") == 0) {
					decodeFontChr(fp);
					return 0;
				} else if (strcmp(ext, ".BIN") == 0) {
					decodeTextBin(fp);
					renderTextTga();
					fseek(fp, 0, SEEK_SET);
					return 0;
				}
			}
			char buf[4];
			fread(buf, 4, 1, fp);
			fseek(fp, 0, SEEK_SET);

			if (memcmp(buf, "ANIM", 4) == 0) {
				decodeAnim(fp);
				return 0;
			} else if (memcmp(buf, "CCB ", 4) == 0) {
				decodeCel(fp, "ccb.tga");
				return 0;
			} else if (memcmp(buf, "SHDR", 4) != 0) {
				fprintf(stderr, "Unhandled file '%s'\n", argv[1]);
				return -1;
			}
			// .cpc
			CinepakDecoder decoder;
			OutputBuffer out;
			int frmeCounter = 0;
			while (1) {
				uint32_t pos = ftell(fp);
				char tag[4];
				uint32_t size = readTag(fp, tag);
				if (feof(fp)) {
					break;
				}
				if (memcmp(tag, "FILM", 4) == 0) {
					fseek(fp, 8, SEEK_CUR);
					char type[4];
					fread(type, 4, 1, fp);
					if (memcmp(type, "FHDR", 4) == 0) {
						fseek(fp, 4, SEEK_CUR);
						char compression[4];
						fread(compression, 4, 1, fp);
						uint32_t height = freadUint32BE(fp);
						uint32_t width = freadUint32BE(fp);
						out.setup(width, height, &decoder);
					} else if (memcmp(type, "FRME", 4) == 0) {
						uint32_t duration = freadUint32BE(fp);
						uint32_t frameSize = freadUint32BE(fp);
						uint8_t *frameBuf = (uint8_t *)malloc(frameSize);
						if (frameBuf) {
							fread(frameBuf, 1, frameSize, fp);
							// fprintf(stdout, "FRME duration %d frame %d size %d\n", duration, frameSize, size);
							decoder.decode(frameBuf, frameSize);
							free(frameBuf);
							out.dump(frmeCounter);
						}
						++frmeCounter;
					}
				} else if (memcmp(tag, "SHDR", 4) == 0) {
					// ignore
				} else if (memcmp(tag, "SNDS", 4) == 0) {
					// ignore
				} else if (memcmp(tag, "FILL", 4) == 0) {
					// ignore
				} else {
					fprintf(stderr, "Unhandled tag '%c%c%c%c' size %d\n", tag[0], tag[1], tag[2], tag[3], size);
				}
				fseek(fp, pos + size, SEEK_SET);
			}
			fclose(fp);
		}
	}
	return 0;
}
