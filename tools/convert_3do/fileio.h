
#ifndef FILEIO_H__
#define FILEIO_H__

int fileReadUint16LE(FILE *fp);
int fileReadUint32LE(FILE *fp);
int fileWriteUint16LE(FILE *fp, int n);

#endif // FILEIO_H__
