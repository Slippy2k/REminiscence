#include <stdio.h>

/* FlashBack Amiga_EN _creditsData */
static const int kPos = 0x29D48;
static const int kEnd = 0x2A6F0;

int main(int argc, char *argv[]) {
	if (argc >= 2) {
		int count = 0;
		FILE *fp = fopen(argv[1], "rb");
		if (fp) {
			fseek(fp, kPos, SEEK_SET);
			while (ftell(fp) <= kEnd) {
				if ((count % 16) == 0) {
					printf("\n\t");
				} else {
					printf(" ");
				}
				printf("0x%02X,", fgetc(fp));
				++count;
			}
			fclose(fp);
		}
	}
	return 0;
}
