#include "psx_io.h"
#include "internal.h"
#include "../sys.h"

#include <stdio.h>
#include <string.h>

static char const * const pak_filename = "./id1/pak0.pak";

static struct PQMcFile mc_files[PSX_MEMCARD_FILES_MAX] = { 0 };

struct PQMcFile * pq_mc_file_alloc(void)
{
    EnterCriticalSection();
    for (int i = 0; i < ARRAY_SIZE(mc_files); ++i) {
        if (!mc_files[i].allocated) {
            memset(&mc_files[i], 0, sizeof(*mc_files));
            mc_files[i].allocated = 1;
            ExitCriticalSection();
            return &mc_files[i];
        }
    }
    ExitCriticalSection();
    return NULL;
}

static FILE * fopen_mc_file(uint16_t filename_hash)
{
    // Quake uses fopen to get a second file handle for pak files
    for (int i = 0; i < ARRAY_SIZE(pq_cd_files); ++i) {
        struct PQCdFile * f = &pq_cd_files[i];
        if (f->allocated && f->filename_hash == filename_hash) {
            struct PQCdFile * ret = pq_cd_file_alloc();
            memcpy(ret, f, sizeof(*f));
            return (void*)ret;
        }
    }
    printf("TODO fopen_cd_file unknown file\n");
    return 0;
}

static FILE * fopen_cd_file(uint16_t filename_hash)
{
    // Quake uses fopen to get a second file handle for pak files
    for (int i = 0; i < ARRAY_SIZE(pq_cd_files); ++i) {
        struct PQCdFile * f = &pq_cd_files[i];
        if (f->allocated && f->filename_hash == filename_hash) {
            // If we have the file already open then allocate a new handle and copy the contents
            struct PQCdFile * ret = pq_cd_file_alloc();
            memcpy(ret, f, sizeof(*f));
            f->cursor_sectors = 0;
            f->cursor_bytes = 0;
            return (void*)ret;
        }
    }
    printf("TODO fopen_cd_file unknown file\n");
    return 0;
}

FILE * fopen (const char * filename, const char * mode)
{
    size_t filename_len = strlen(filename);
    uint16_t search_hash = crc16(0, filename, filename_len);
    printf("fopen %s %s\n", filename, mode);

    if (strchr(mode, 'r')) {
        return fopen_cd_file(search_hash);
    } else if (strchr(mode, 'w')) {
        // return fopen_mc_file(search_hash);
    }

    printf("Unsupported mode in fopen: %s\n", mode);
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
