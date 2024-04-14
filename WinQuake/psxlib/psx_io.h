#ifndef PSX_IO_H
#define PSX_IO_H

#include <stdlib.h>
#include <psxapi.h>

#define strerror(e) ""

typedef int FILE;

FILE * fopen(const char * filename, const char * mode);

int fseek(FILE * stream, long int offset, int origin);

int fclose(FILE *stream);

size_t fread(void * ptr, size_t size, size_t nmemb, FILE *restrict stream);

size_t fwrite(const void * ptr, size_t size, size_t nmemb, FILE *restrict stream);

int fprintf(FILE *restrict stream, const char *restrict format, ...);

int fflush(FILE * stream);

int fscanf(FILE *restrict stream, const char *restrict format, ...);

int unlink(char const * pathname);

#define O_WRONLY 1
#define O_CREAT 2
#define O_APPEND 4
#define EOF -1

int atoi(const char *nptr);
double atof(char const * nptr);

#endif
