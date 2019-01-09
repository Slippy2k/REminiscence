
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cinepak.h"
#include "decode_3dostr.h"
#include "fileio.h"
extern "C" {
#include "tga.h"
}

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

static uint16_t yuv_to_rgb555(int y, int u, int v) {
	int r = int(y + (1.370705 * (v - 128)));
	r = clip8(r) >> 3;
	int g = int(y - (0.698001 * (v - 128)) - (0.337633 * (u - 128)));
	g = clip8(g) >> 3;
	int b = int(y + (1.732446 * (u - 128)));
	b = clip8(b) >> 3;
	return (r << 10) | (g << 5) | b;
}

static void uyvy_to_rgb555(const uint8_t *in, int len, uint16_t *out) {
	assert((len & 3) == 0);
	for (int i = 0; i < len; i += 4, in += 4) {
		const int u  = in[0];
		const int y0 = in[1];
		const int v  = in[2];
		const int y1 = in[3];
		*out++ = yuv_to_rgb555(y0, u, v);
		*out++ = yuv_to_rgb555(y1, u, v);
	}
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
			uyvy_to_rgb555(_buf, _bufSize, (uint16_t *)_rgbBuffer);
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

int decode_3dostr(FILE *fp) {
	char buf[4];
	fread(buf, 4, 1, fp);
	fseek(fp, 0, SEEK_SET);
	if (memcmp(buf, "SHDR", 4) != 0) {
		return -1;
	}
	// .cpak
	CinepakDecoder decoder;
	OutputBuffer out;
	int frmeCounter = 0;
	while (1) {
		uint32_t pos = ftell(fp);
		char tag[4];
		uint32_t size = freadTag(fp, tag);
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
}
