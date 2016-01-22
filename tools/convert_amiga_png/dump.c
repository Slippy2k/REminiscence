
#include <stdio.h>

static const int baseFileOffset = 0x2CE36 - 0x70672;

#define DEFAULT_SIZE 66

static void dumpTable(FILE *fp, const char *name, int startOffset, int endOffset) {
	int i, size = endOffset - startOffset;
	size = DEFAULT_SIZE;
	fseek(fp, baseFileOffset + startOffset, SEEK_SET);
	printf("static const unsigned char %s[%d] = {", name, size);
	for (i = 0; i < size; ++i) {
		if (i % 16 == 0) {
			printf("\n\t");
		}
		printf("0x%02X, ", fgetc(fp));
	}
	printf("\n};\n");
}

static void dumpStringsTable(FILE *fp, int startOffset, int endOffset) {
	char name[64];
	fseek(fp, startOffset, SEEK_SET);
	int count = 0;
	while (ftell(fp) < endOffset) {
		int i = 0;
		while ((name[i] = fgetc(fp)) != 0) {
			++i;
		}
		printf("num %d name %s\n", count, name);
		++count;
		fgetc(fp); // skip extra '0'
	}
}

int main(int argc, char *argv[]) {
	FILE *fp = fopen("../../DATA/DATA_amiga/flashback", "r");
	if (fp) {
		dumpTable(fp, "_spl_1_table", 0x70672, 0x70774);
		dumpTable(fp, "_spl_2_table", 0x70774, 0x70876);
		dumpTable(fp, "_spl_dt_table", 0x70876, 0x70978);
		dumpTable(fp, "_spl_3_1_table", 0x70978, 0x70A7A);
		dumpTable(fp, "_spl_3_2_table", 0x70A7A, 0x70B7C);
		dumpTable(fp, "_spl_4_1_table", 0x70B7C, 0x70C7E);
		dumpTable(fp, "_spl_4_2_table", 0x70C7E, 0x70C7E + 258);
		dumpStringsTable(fp, 0x2C8A5, 0x2CCC2);
		fclose(fp);
	}
	return 0;
}

