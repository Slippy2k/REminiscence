#include <stdio.h>

int main(int argc, char *argv[]) {
	if (argc >= 2) {
		int count = 0;
		FILE *fp = fopen(argv[1], "rb");
		if (fp) {
			fseek(fp, 0x1D508, SEEK_SET);
			while (ftell(fp) <= 0x1E31B) {
				if ((count % 16) == 0) {
					printf("\n\t");
				}
				printf("0x%02X, ", fgetc(fp));
				++count;
			}
			fclose(fp);
		}
	}
	return 0;
}
