
#ifndef FILEIO_H__
#define FILEIO_H__

uint8_t *readFile(FILE *fp, int *size);

uint16_t freadUint16LE(FILE *fp);
uint16_t freadUint16BE(FILE *fp);
uint32_t freadUint32BE(FILE *fp);
uint32_t freadTag(FILE *fp, char *type);

void fwriteUint16LE(FILE *fp, uint16_t value);
void fwriteUint32LE(FILE *fp, uint32_t value);

#endif
