#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <stdio.h>
#include <stdlib.h>
#include <zstd.h>

// Struct to manage the Zstandard compression stream
typedef struct {
    FILE *file;
    ZSTD_CStream *zstream;
    void *outBuffer;
    size_t outBufferSize;
} ZstdFile;

ZstdFile* zstdOpen(const char *filename, int compressionLevel);
size_t zstdWrite(ZstdFile *zfile, const void *data, size_t dataSize);
int zstdClose(ZstdFile *zfile);

#endif