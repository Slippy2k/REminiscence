
#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libmodplug/modplug.h>
#include "file.h"

static const char *OUT = "out";

static uint8_t *readFile(const char *path, int *size) {
	uint8_t *tmp = 0;
	FILE *fp = fopen(path, "rb");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		*size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		tmp = (uint8_t *)malloc(*size);
		if (tmp) {
			const int count = fread(tmp, 1, *size, fp);
			if (count != *size) {
				fprintf(stderr, "Failed to read %d bytes (%d)\n", *size,  count);
				free(tmp);
				tmp = 0;
			}
		}
		fclose(fp);
	}
	return tmp;
}

static const uint16_t voiceOffsetsTable[] = {
	0x0064, 0x006A, 0x0070, 0x0084, 0x0090, 0x0096, 0x009E, 0x00A4, 0x00AE, 0x00B4,
	0x00BC, 0x00C4, 0x00CC, 0x00D4, 0x00E0, 0x00E6, 0x00EC, 0x00F6, 0x00FC, 0x0102,
	0x010C, 0x0126, 0x0130, 0x0136, 0x013E, 0x0144, 0x014C, 0x0152, 0x015A, 0x0160,
	0x0166, 0x016C, 0x0176, 0x017C, 0x0186, 0x018C, 0x0198, 0x019E, 0x01A4, 0x01AC,
	0x01B6, 0x01BE, 0x01C6, 0x01CC, 0x01D4, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	0x0000, 0x0001, 0x0073, 0x0073, 0x0001, 0x0050, 0x00C3, 0x0008, 0x00D7, 0x007D,
	0x0078, 0x007D, 0x0087, 0x005A, 0x00E1, 0x0087, 0x0555, 0x0004, 0x00D7, 0x0069,
	0x00EB, 0x00A5, 0x0825, 0x0001, 0x008C, 0x08B1, 0x0002, 0x00CD, 0x0073, 0x09F1,
	0x0001, 0x0113, 0x0B04, 0x0003, 0x0113, 0x008C, 0x00B9, 0x0D5C, 0x0001, 0x0096,
	0x0DF2, 0x0002, 0x00AF, 0x0082, 0x0F23, 0x0002, 0x0069, 0x009B, 0x1027, 0x0002,
	0x0069, 0x00EB, 0x117B, 0x0002, 0x0069, 0x00C3, 0x12A7, 0x0004, 0x0064, 0x00AA,
	0x0078, 0x00DC, 0x1509, 0x0001, 0x00B4, 0x15BD, 0x0001, 0x0046, 0x1603, 0x0003,
	0x0050, 0x0118, 0x00A5, 0x1810, 0x0001, 0x0041, 0x1851, 0x0001, 0x0082, 0x18D3,
	0x0003, 0x0104, 0x00D7, 0x00AF, 0x1B5D, 0x000B, 0x00AA, 0x00BE, 0x0127, 0x005A,
	0x00F5, 0x00AA, 0x0104, 0x00BE, 0x00DC, 0x00B9, 0x00D7, 0x2413, 0x0003, 0x00C3,
	0x00E6, 0x003C, 0x25F8, 0x0001, 0x0069, 0x2661, 0x0002, 0x00BE, 0x00AF, 0x27CE,
	0x0001, 0x0050, 0x281E, 0x0002, 0x0041, 0x0037, 0x2896, 0x0001, 0x011D, 0x29B3,
	0x0002, 0x0041, 0x003C, 0x2A30, 0x0001, 0x00A5, 0x2AD5, 0x0001, 0x0000, 0x2AD5,
	0x0001, 0x0000, 0x2AD5, 0x0003, 0x005A, 0x00B9, 0x0073, 0x2C5B, 0x0001, 0x005F,
	0x2CBA, 0x0003, 0x00DC, 0x0064, 0x00F0, 0x2EEA, 0x0001, 0x00B9, 0x2FA3, 0x0004,
	0x0181, 0x013B, 0x005F, 0x0154, 0x3412, 0x0001, 0x00AF, 0x34C1, 0x0001, 0x00A0,
	0x3561, 0x0002, 0x0069, 0x003C, 0x3606, 0x0003, 0x0000, 0x00FA, 0x00F0, 0x37F0,
	0x0002, 0x006E, 0x00A0, 0x38FE, 0x0002, 0x0064, 0x00E6, 0x3A48, 0x0001, 0x00A5,
	0x3AED, 0x0002, 0x0078, 0x0087, 0x3BEC, 0x0002, 0x00AA, 0x00F0
};

static void decodeSampleToS8(uint8_t *data, int len) {
	for (int i = 0; i < len; ++i) {
		if (data[i] & 0x80) {
			const int s = -(data[i] & 0x7F);
			data[i] = (uint8_t)(s & 255);
		}
	}
}

static void convertVoiceToWav(File &f) {
	static const int kSectorSize = 2048;
	for (int num = 0; voiceOffsetsTable[num] != 0xFFFF; ++num) {
		const uint16_t *p = voiceOffsetsTable + voiceOffsetsTable[num] / 2;
		int offset = p[0] * kSectorSize; // first sector
		int count = p[1]; // number of segments
		offset += kSectorSize * 4; // 4 sectors padding
		for (int segment = 0; segment < count; ++segment) {
			const int len = p[2 + segment]; // number of sectors
			if (len == 0) {
				continue;
			}
			char path[MAXPATHLEN];
			snprintf(path, sizeof(path), "%s/v%02d_%02d.raw", OUT, num, segment);
			File out;
			if (!out.open(path, "wb")) {
				fprintf(stderr, "Fail to open '%s' for writing\n", path);
				continue;
			}
			for (int i = 0; i < len / 5; ++i) {
				f.seek(offset);
				uint8_t sector[kSectorSize];
				f.read(sector, sizeof(sector));
				decodeSampleToS8(sector, sizeof(sector));
				out.write(sector, sizeof(sector));
				offset += kSectorSize * 5; // 4 sectors padding
			}
			out.close();
			char wav[MAXPATHLEN];
			snprintf(wav, sizeof(wav), "%s/v%02d_%02d.wav", OUT, num, segment);
			char cmd[1024];
			snprintf(cmd, sizeof(cmd), "sox -r 32000 -e signed -b 8 -c 1 %s rate 11025 %s", path, wav);
			const int ret = system(cmd);
			if (ret < 0) {
				fprintf(stderr, "Failed to convert .raw to .wav (%s)\n", cmd);
			}
		}
	}
}

static void convertAmigaModToWav(const char *name, const uint8_t *data, int size) {
	ModPlugFile *f = ModPlug_Load(data, size);
	if (!f) {
		fprintf(stderr, "Failed to load '%s'\n", name);
		return;
	}
	fprintf(stdout, "Music '%s' size %d, name '%s'\n", name, size, ModPlug_GetName(f));

	ModPlug_Settings s;
	memset(&s, 0, sizeof(s));
	ModPlug_GetSettings(&s);
	s.mFlags = MODPLUG_ENABLE_OVERSAMPLING | MODPLUG_ENABLE_NOISE_REDUCTION;
	s.mChannels = 2;
	s.mBits = 16;
	s.mFrequency = 22050;
	s.mResamplingMode = MODPLUG_RESAMPLE_FIR;
	ModPlug_SetSettings(&s);

	FILE *fp = fopen("out.raw", "wb");
	if (fp) {
		int count;
		uint8_t buf[4096];
		while ((count = ModPlug_Read(f, buf, sizeof(buf))) > 0) {
			fwrite(buf, 1, count, fp);
		}
		fclose(fp);
		char cmd[1024];
		snprintf(cmd, sizeof(cmd), "sox -r 22050 -e signed -b 16 -c 2 out.raw %s/%s.wav", OUT, name);
		const int ret = system(cmd);
		if (ret < 0) {
			fprintf(stderr, "Failed to convert .raw to .wav (%s)\n", cmd);
		}
		unlink("out.raw");
	}

	ModPlug_Unload(f);
}

static void convertAmigaModDir(const char *dir) {
	DIR *d = opendir(dir);
	if (d) {
		dirent *de;
		while ((de = readdir(d)) != NULL) {
			if (de->d_name[0] == '.') {
				continue;
			}
			char path[MAXPATHLEN];
			snprintf(path, sizeof(path), "%s/%s", dir, de->d_name);
			int size;
			uint8_t *data = readFile(path, &size);
			if (!data) {
				fprintf(stderr, "Failed to read '%s'\n", path);
				continue;
			}
			convertAmigaModToWav(de->d_name, data, size);
			free(data);
		}
		closedir(d);
	}
}

int main(int argc, char *argv[]) {
	while (1) {
		static struct option options[] = {
			{ "voice", required_argument, 0, 1 },
			{ "music", required_argument, 0, 2 },
			{ 0, 0, 0, 0 }
		};
		int index;
		const int c = getopt_long(argc, argv, "", options, &index);
		if (c == -1) {
			break;
		}
		switch (c) {
		case 1:
			{
				File f;
				if (!f.open(optarg, "rb")) {
					fprintf(stderr, "Unable to open '%s' fo reading\n", optarg);
				} else {
					convertVoiceToWav(f);
				}
			}
			break;
		case 2:
			{
				struct stat st;
				if (stat(optarg, &st) != 0 || !S_ISDIR(st.st_mode)) {
					fprintf(stderr, "'%s' is not a directory\n", optarg);
				} else {
					convertAmigaModDir(optarg);
				}
			}
			break;
		}
	}
	return 0;
}
