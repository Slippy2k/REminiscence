
#include "bitmap.h"
#include "file.h"

static const uint16_t TAG_BM = 0x4D42;

void WriteFile_BMP_PAL(const char *filename, int w, int h, int pitch, const uint8_t *bits, const uint8_t *pal) {
	FILE *fp = fopen(filename, "wb");
	if (fp) {
		int alignWidth = (w + 3) & ~3;
		int imageSize = alignWidth * h;

		// Write file header
		fwriteUint16LE(fp, TAG_BM);
		fwriteUint32LE(fp, 14 + 40 + 4 * 256 + imageSize);
		fwriteUint16LE(fp, 0); // reserved1
		fwriteUint16LE(fp, 0); // reserved2
		fwriteUint32LE(fp, 14 + 40 + 4 * 256);

		// Write info header
		fwriteUint32LE(fp, 40);
		fwriteUint32LE(fp, w);
		fwriteUint32LE(fp, h);
		fwriteUint16LE(fp, 1); // planes
		fwriteUint16LE(fp, 8); // bit_count
		fwriteUint32LE(fp, 0); // compression
		fwriteUint32LE(fp, imageSize); // size_image
		fwriteUint32LE(fp, 0); // x_pels_per_meter
		fwriteUint32LE(fp, 0); // y_pels_per_meter
		fwriteUint32LE(fp, 0); // num_colors_used
		fwriteUint32LE(fp, 0); // num_colors_important

		// Write palette data
		for (int i = 0; i < 256; ++i) {
			fwriteByte(fp, pal[2]);
			fwriteByte(fp, pal[1]);
			fwriteByte(fp, pal[0]);
			fwriteByte(fp, 0);
			pal += 3;
		}

		// Write bitmap data
		bits += h * pitch;
		for (int i = 0; i < h; ++i) {
			bits -= pitch;
			fwrite(bits, w, 1, fp);
			int pad = alignWidth - w;
			while (pad--) {
				fwriteByte(fp, 0);
			}
		}
		
		fclose(fp);
	}
}

void WriteFile_TGA_RGB(const char *filename, int w, int h, int pitch, const uint32_t *bits) {
	FILE *fp = fopen(filename, "wb");
	if (fp) {
		fputc(0, fp); // 0, ID Length
		fputc(0, fp); // 1, ColorMap Type
		fputc(2, fp); // 2, Image Type (uncompressed true color)
		fwriteUint16LE(fp, 0); // 3, colourmapstart
		fwriteUint16LE(fp, 0); // 5, colourmaplength
		fputc(0, fp); // 7, colourmapbits
		fwriteUint16LE(fp, 0); // 8, xstart
		fwriteUint16LE(fp, 0); // 10, ystart
		fwriteUint16LE(fp, w); // 12
		fwriteUint16LE(fp, h); // 14
		fputc(32, fp); // 16, bitdepth
		fputc((1 << 5), fp); // 17, descriptor (top)
		for (int i = 0; i < h; ++i) {
			fwrite(bits, w * sizeof(uint32_t), 1, fp);
			bits += pitch;
		}
		fclose(fp);
	}
}
