
#include <arpa/inet.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cinepak.h"

struct OutputBuffer {
	void setup(int w, int h, CinepakDecoder *decoder) {
		_bufSize = w * h * 2;
		_buf = (uint8_t *)malloc(_bufSize);
		decoder->_yuvFrame = _buf;
		decoder->_yuvW = w;
		decoder->_yuvH = h;
		decoder->_yuvPitch = w * 2;
	}
	void dump(int num) {
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
};

static uint32_t readInt(FILE *fp) {
	uint32_t i;
	fread(&i, sizeof(i), 1, fp);
	return htonl(i);
}

static uint32_t readTag(FILE *fp, char *type) {
	fread(type, 4, 1, fp);
	uint32_t size;
	fread(&size, sizeof(size), 1, fp);
	return htonl(size);
}

int main(int argc, char *argv[]) {
	if (argc == 2) {
		FILE *fp = fopen(argv[1], "rb");
		if (fp) {
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
						uint32_t height = readInt(fp);
						uint32_t width = readInt(fp);
						out.setup(width, height, &decoder);
					} else if (memcmp(type, "FRME", 4) == 0) {
						uint32_t duration = readInt(fp);
						uint32_t frameSize = readInt(fp);
						uint8_t *frameBuf = (uint8_t *)malloc(frameSize);
						if (frameBuf) {
							fread(frameBuf, 1, frameSize, fp);
							printf("FRME duration %d frame %d size %d\n", duration, frameSize, size);
							decoder.decode(frameBuf, frameSize);
							free(frameBuf);
							out.dump(frmeCounter);
						}
						++frmeCounter;
					}
				}
				fseek(fp, pos + size, SEEK_SET);
			}
			fclose(fp);
		}
	}
	return 0;
}
