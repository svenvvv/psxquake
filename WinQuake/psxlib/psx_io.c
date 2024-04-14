#include "psx_io.h"
#include "internal.h"
#include "../sys.h"

#include <stdio.h>
#include <string.h>

static char const * const pak_filename = "./id1/pak0.pak";

FILE * fopen (const char * filename, const char * mode)
{
    size_t filename_len = strlen(filename);
    printf("fopen %s %s\n", filename, mode);

    uint16_t search_hash = crc16(0, filename, filename_len);
    for (int i = 0; i < ARRAY_SIZE(pq_cd_files); ++i) {
        struct PQCdFile * f = &pq_cd_files[i];
        if (f->allocated && f->filename_hash == search_hash) {
            if (strchr(mode, 'w')) {
                // Can't write to CD
                return 0;
            }

            struct PQCdFile * ret = pq_cd_file_alloc();
            memcpy(ret, f, sizeof(*f));
            return (void*)ret;
        }
    }

    return 0;
}

int fscanf(FILE *restrict stream, const char *restrict format, ...)
{
    printf("fscanf\n");
    return 0;
}

int fprintf(FILE *restrict stream, const char *restrict format, ...)
{
    return 0;
}

int fclose(FILE *stream)
{
    struct PQCdFile * f = (void*)stream;
    if (!pq_cd_file_is_valid(f)) {
        printf("fseek invalid file\n");
        return -1;
    }

    Sys_FileClose((int)f);

    return 0;
}

int feof(FILE *stream)
{
    printf("feof\n");
}

int fseek(FILE *stream, long offset, int whence)
{
    struct PQCdFile * f = (void*)stream;
    if (!pq_cd_file_is_valid(f)) {
        printf("fseek invalid file\n");
        return -1;
    }
    printf("fseek %u %u\n", offset, whence);

    Sys_FileSeek((int)f, offset);

    return 0;
}

size_t fread(void * ptr, size_t size, size_t nmemb, FILE *restrict stream)
{
    struct PQCdFile * f = (void*)stream;

    if (!pq_cd_file_is_valid(f)) {
        printf("fread invalid file\n");
        return EOF;
    }
    printf("fread %u %u\n", size, nmemb);
    return Sys_FileRead((int)f, ptr, size * nmemb);
}

size_t fwrite(const void * ptr, size_t size, size_t nmemb, FILE *restrict stream)
{
    return 0;
}

int fflush(FILE * stream)
{
    return 0;
}

int unlink(char const * pathname)
{
    return 0;
}

int fgetc(FILE * stream)
{
    int err;
    uint8_t ret;
    struct PQCdFile * f = (void*)stream;

    if (!pq_cd_file_is_valid(f)) {
        printf("fread invalid file\n");
        return EOF;
    }

    printf("fgetc\n");

    err = Sys_FileRead((int)f, &ret, sizeof(ret));
    if (err != sizeof(ret)) {
        return EOF;
    }

    return ret;
}
