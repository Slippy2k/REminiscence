
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct Base64Encoder {
	uint32_t _bits;
	int _count;

	int getCode(int code) {
		switch (code) {
		case  0 ... 25:
			return 'A' + code;
		case 26 ... 51:
			return 'a' + code - 26;
		case 52 ... 61:
			return '0' + code - 52;
		case 62:
			return '+';
		}
		return '/';
	}

	void encode(int chr) {
		_bits <<= 8;
		_bits |= chr;
		_count += 8;
		if (_count == 24) {
			fprintf(stdout, "%c", getCode((_bits >> 18) & 63) );
			fprintf(stdout, "%c", getCode((_bits >> 12) & 63) );
			fprintf(stdout, "%c", getCode((_bits >>  6) & 63) );
			fprintf(stdout, "%c", getCode( _bits        & 63) );
			_bits = 0;
			_count = 0;
		}
	}
	void finish() {
		switch (_count) {
		case 16:
			_bits <<= 2;
			fprintf(stdout, "%c", getCode((_bits >> 12) & 63) );
			fprintf(stdout, "%c", getCode((_bits >>  6) & 63) );
			fprintf(stdout, "%c", getCode( _bits        & 63) );
			fprintf(stdout, "=");
			break;
		case 8:
			_bits <<= 4;
			fprintf(stdout, "%c", getCode((_bits >>  6) & 63) );
			fprintf(stdout, "%c", getCode( _bits        & 63) );
			fprintf(stdout, "==");
			break;
		}
	}
};

static void encode(const char *path) {
	FILE *fp = fopen(path, "rb");
	if (fp) {
		Base64Encoder encoder;
		memset(&encoder, 0, sizeof(encoder));
		while (1) {
			const int c = fgetc(fp);
			if (feof(fp)) {
				break;
			}
			encoder.encode(c);
		}
		encoder.finish();
		fclose(fp);
	}
}

int main(int argc, char *argv[]) {
	if (argc >= 3) {
		fprintf(stdout, "var g_pol = '");
			encode(argv[1]);
		fprintf(stdout, "';\n");
		fprintf(stdout, "var g_cmd = '");
			encode(argv[2]);
		fprintf(stdout, "';\n");
	}
	return 0;
}
