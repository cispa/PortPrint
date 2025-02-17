#include <string.h>
#include "compression.h"

// Function to open a Zstandard-compressed file for writing
ZstdFile* zstdOpen(const char *filename, int compressionLevel) {
    ZstdFile *zfile = malloc(sizeof(ZstdFile));
    if (!zfile) {
        perror("Failed to allocate memory for ZstdFile");
        return NULL;
    }

    zfile->file = fopen(filename, "wb");
    if (!zfile->file) {
        perror("Failed to open file");
        free(zfile);
        return NULL;
    }

    zfile->zstream = ZSTD_createCStream();
    if (!zfile->zstream) {
        perror("Failed to create ZSTD_CStream");
        fclose(zfile->file);
        free(zfile);
        return NULL;
    }

    size_t initResult = ZSTD_initCStream(zfile->zstream, compressionLevel);
    if (ZSTD_isError(initResult)) {
        fprintf(stderr, "ZSTD_initCStream error: %s\n", ZSTD_getErrorName(initResult));
        ZSTD_freeCStream(zfile->zstream);
        fclose(zfile->file);
        free(zfile);
        return NULL;
    }

    zfile->outBufferSize = ZSTD_CStreamOutSize();
    zfile->outBuffer = malloc(zfile->outBufferSize);
    if (!zfile->outBuffer) {
        perror("Failed to allocate output buffer");
        ZSTD_freeCStream(zfile->zstream);
        fclose(zfile->file);
        free(zfile);
        return NULL;
    }

    return zfile;
}

// Function to write data to a Zstandard-compressed file
size_t zstdWrite(ZstdFile *zfile, const void *data, size_t dataSize) {
    ZSTD_inBuffer input = { data, dataSize, 0 };
    while (input.pos < input.size) {
        ZSTD_outBuffer output = { zfile->outBuffer, zfile->outBufferSize, 0 };
        size_t ret = ZSTD_compressStream(zfile->zstream, &output, &input);
        if (ZSTD_isError(ret)) {
            fprintf(stderr, "ZSTD_compressStream error: %s\n", ZSTD_getErrorName(ret));
            return ret;
        }

        if (fwrite(zfile->outBuffer, 1, output.pos, zfile->file) != output.pos) {
            perror("Failed to write to file");
            return -1;
        }
    }
    return dataSize;
}

// Function to close a Zstandard-compressed file
int zstdClose(ZstdFile *zfile) {
    ZSTD_outBuffer output = { zfile->outBuffer, zfile->outBufferSize, 0 };
    size_t ret;
    do {
        ret = ZSTD_endStream(zfile->zstream, &output);
        if (ZSTD_isError(ret)) {
            fprintf(stderr, "ZSTD_endStream error: %s\n", ZSTD_getErrorName(ret));
            return -1;
        }

        if (fwrite(zfile->outBuffer, 1, output.pos, zfile->file) != output.pos) {
            perror("Failed to write to file during finalization");
            return -1;
        }
    } while (ret > 0);

    fclose(zfile->file);
    ZSTD_freeCStream(zfile->zstream);
    free(zfile->outBuffer);
    free(zfile);
    return 0;
}
