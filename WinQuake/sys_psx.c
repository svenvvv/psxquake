#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "quakedef.h"

#include <psxgpu.h>
#include <psxapi.h>
#include <psxcd.h>
#include "psxlib/internal.h"

#define SCREEN_XRES	320
#define SCREEN_YRES	240

static uint32_t systick_ms;
static uint8_t ms_per_frame;

qboolean            isDedicated;

int nostdout = 0;

char * basedir = ".";
char * cachedir = "/tmp";

cvar_t  sys_linerefresh = {"sys_linerefresh", "0"}; // set for entity display

// =======================================================================
// General routines
// =======================================================================

void Sys_DebugNumber(int y, int val)
{
}

#include <stdarg.h>
void Sys_Printf(char * fmt, ...)
{
    va_list     argptr;
    static char        text[256];
    unsigned char    *   p;

    va_start(argptr, fmt);
    vsnprintf(text, sizeof(text), fmt, argptr);
    va_end(argptr);

	printf("Sys_Printf: ");
    for (p = (unsigned char *)text; *p; p++) {
        *p &= 0x7f;

        if ((*p > 128 || *p < 32) && *p != 10 && *p != 13 && *p != 9)
            printf("[%02x]", *p);
        else
			printf("%c", *p);
    }
}

void Sys_Quit(void)
{
    Host_Shutdown();
    // fflush(stdout);
    // exit(0);
}

void Sys_Init(void)
{
#if id386
    Sys_SetFPCW();
#endif
}

void Sys_Error(char * error, ...)
{
    va_list     argptr;
    char        string[1024];

    va_start(argptr, error);
    vsnprintf(string, sizeof(string), error, argptr);
    va_end(argptr);
    printf("Sys_Error: %s\n", string);

    Host_Shutdown();

	for (;;) {
	}
}

void Sys_Warn(char * warning, ...)
{
    va_list     argptr;
    char        string[1024];

    va_start(argptr, warning);
    vsnprintf(string, sizeof(string), warning, argptr);
    va_end(argptr);
    printf("Sys_Warn: %s\n", string);
}

/*
============
Sys_FileTime

returns -1 if not present
============
*/
int Sys_FileTime(char * path)
{
    // struct  stat    buf;
    //
    // if (stat(path, &buf) == -1)
    //     return -1;
    //
    // return buf.st_mtime;
	printf("Sys_FileTime %s\n", path);
	return -1;
}

void Sys_mkdir(char * path)
{
    // mkdir(path, 0777);
	printf("Sys_mkdir %s\n", path);
}

struct PQCdFile pq_cd_files[PSX_CD_FILES_MAX] = { 0 };

struct PQCdFile * pq_cd_file_alloc(void)
{
	struct PQCdFile * ret = NULL;
	// EnterCriticalSection();
	for (int i = 0; i < ARRAY_SIZE(pq_cd_files); ++i) {
		if (pq_cd_files[i].allocated) {
			continue;
		}
		ret = &pq_cd_files[i];
		memset(ret, 0, sizeof(*ret));
		ret->allocated = true;
		break;
	}
	// ExitCriticalSection();
	return ret;
}

int Sys_FileOpenRead(char * path, int * handle)
{
	int err;
	struct PQCdFile * f = NULL;
	char * tmp = path;
	char pathbuf[64];
	size_t pathbuf_len = 0;

	if (*tmp == '.') {
		tmp += 1;
	}
	while (*tmp != 0) {
		if (*tmp == '/') {
			pathbuf_len = 0;
			// pathbuf[pathbuf_len++] = '\\';
		} else {
			pathbuf[pathbuf_len++] = toupper(*tmp);
		}
		tmp += 1;
	}
	pathbuf[pathbuf_len++] = 0;

	f = pq_cd_file_alloc();
	if (f == NULL) {
		Sys_Error("All out of CD file handles..");
		goto error;
	}

	printf("Sys_FileOpenRead 0x%p %s\n", f, pathbuf);

	if (CdSearchFile(&f->file, pathbuf) == NULL) {
		Sys_Warn("Failed to open file %s", path);
		f->allocated = false;
		goto error;
	}

	f->filename_hash = crc16(0, (uint8_t const*) path, strlen(path));
	*handle = (int)f;
	return f->file.size;
error:
	*handle = -1;
	return -1;
}

int Sys_FileOpenWrite(char * path)
{
	printf("Sys_FileOpenWrite %s", path);
    // int     handle;
    //
    // umask(0);
    //
    // handle = open(path, O_RDWR | O_CREAT | O_TRUNC , 0666);
    //
    // if (handle == -1)
    //     Sys_Error("Error opening %s: %s", path, strerror(errno));
    //
    // return handle;
	return 0;
}

int Sys_FileWrite(int handle, void const * src, int count)
{
	printf("Sys_FileWrite\n", count);
    // return write(handle, src, count);
	return 0;
}

int pq_cd_file_is_valid(struct PQCdFile const * f)
{
	return pq_cd_files <= f || f < &pq_cd_files[ARRAY_SIZE(pq_cd_files)];
}

void Sys_FileClose(int handle)
{
	struct PQCdFile * f = (struct PQCdFile *)handle;

	if (!pq_cd_file_is_valid(f)) {
		Sys_Warn("Invalid file handle access");
		return;
	}

	printf("Sys_FileClose 0x%p\n", f);

	f->allocated = false;
}

#define CD_FILE_SIZE_ROUND(sz) ((sz + 2047U) & 0xfffff800)

void Sys_FileSeek(int handle, int position)
{
	struct PQCdFile * f = (struct PQCdFile *) handle;

	printf("Sys_FileSeek 0x%p %u\n", f, position);

	if (!pq_cd_file_is_valid(f)) {
		Sys_Warn("Invalid file handle access");
		return;
	}

	f->cursor_sectors = position / PSX_CD_SECTOR_SIZE;
	f->cursor_bytes = position % PSX_CD_SECTOR_SIZE;
	f->read_buf_is_valid = 0;

	printf("Seek %u, sec %u bytes %u, file len %u\n", position, f->cursor_sectors, f->cursor_bytes, f->file.size);
}

int pq_cd_file_fill_read_buf(struct PQCdFile * f)
{
	CdlLOC loc;

	int position = CdPosToInt(&f->file.pos) + f->cursor_sectors;
	CdIntToPos(position, &loc);
	CdControl(CdlSetloc, &loc, 0);

	CdRead(1, (uint32_t *) f->read_buf, CdlModeSpeed);
	if (CdReadSync(0, 0) < 0) {
		Sys_Error("Failed to read from CD drive");
	}
	f->read_buf_is_valid = true;

	return 0;
}

int Sys_FileRead(int handle, void * dest, int count)
{
	size_t copied = 0;
	struct PQCdFile * f = (struct PQCdFile*) handle;

	if (!pq_cd_file_is_valid(f)) {
		Sys_Warn("Invalid file handle access");
		return 0;
	}

	// TODO we do a double-read for the first partial chunk if this condition is false
	if (f->read_buf_is_valid && f->cursor_bytes + count < sizeof(f->read_buf)) {
		printf("Sys_FileRead buffered 0x%p %u\n", f, count);
		memcpy(dest, f->read_buf + f->cursor_bytes, count);
		f->cursor_bytes += count;
		if (f->cursor_bytes >= PSX_CD_SECTOR_SIZE) {
			f->cursor_sectors += 1;
			f->cursor_bytes -= PSX_CD_SECTOR_SIZE;
			f->read_buf_is_valid = false;
		}
		return count;
	}

	printf("Sys_FileRead drive 0x%p %u\n", f, count);
	while (copied < count) {
		pq_cd_file_fill_read_buf(f);

		// printf("read cursor %u %u sizes %u %u\n",
		// 	   f->cursor_sectors, f->cursor_bytes, copied, count);

		size_t copy_len = PSX_CD_SECTOR_SIZE - f->cursor_bytes;
		if ((copied + copy_len) > count) {
			copy_len = count - copied;
		}
		memcpy((uint8_t*) dest + copied, f->read_buf + f->cursor_bytes, copy_len);

		copied += copy_len;

		f->cursor_bytes += copy_len;
		if (f->cursor_bytes >= PSX_CD_SECTOR_SIZE) {
			f->cursor_sectors += 1;
			f->cursor_bytes -= PSX_CD_SECTOR_SIZE;
			f->read_buf_is_valid = false;
		}
	}

	printf("post read cursor %u %u\n", f->cursor_sectors, f->cursor_bytes);

	return copied;
}

void Sys_DebugLog(char * file, char * fmt, ...)
{
    va_list argptr;
    char data[1024];
    int fd;

    va_start(argptr, fmt);
    vsnprintf(data, sizeof(data), fmt, argptr);
    va_end(argptr);
	printf("Sys_DebugLog: %s", data);
}

void Sys_EditFile(char * filename)
{
	// Unsupported on PSX
}

uint32_t Sys_MilliTime(void)
{
	return systick_ms;
}

// =======================================================================
// Sleeps for microseconds
// =======================================================================

char * Sys_ConsoleInput(void)
{
	// Unsupported on PSX
    return NULL;
}

void Sys_HighFPPrecision(void)
{
}

void Sys_LowFPPrecision(void)
{
}

static void vblank_handler(void)
{
	// printf("vblank %u\n", systick_ms);
	systick_ms += ms_per_frame;
	// FIXME? systick overflow not handled
	// I don't think anyone will play PSX quake for over 49 days
	// if (systick == 0) {
	// }
}

static uint8_t membase[6 * 1024 * 1024];

extern uint8_t _end[];

int main(int c, char ** v)
{
    uint32_t time, oldtime, newtime;
    quakeparms_t parms;
    int j;

	void * heap_addr = _end + 4;
#ifndef PSXQUAKE_2MB_RAM
	// 8MiB RAM
	size_t heap_size = (uint8_t *)(0x80000000 + 0x7ffff8) - _end;
#else
	// 2MiB RAM TODO unsupported
	size_t heap_size = (uint8_t *)(0x80000000 + 0x1ffff8) - _end;
#endif
	printf("Allocating heap at %p, size %u\n", heap_addr, heap_size);
	EnterCriticalSection();
	InitHeap(_end + 4, heap_size);
	ExitCriticalSection();

	printf("Starting up Quake, video mode %s...\n", GetVideoMode() == MODE_PAL ? "PAL" : "NTSC");

	ResetGraph(0);
	// FntLoad(960, 0);

	EnterCriticalSection();
	if (GetVideoMode() == MODE_PAL) {
		ms_per_frame = 20; // 1000 / 50 = 20.0
	} else {
		ms_per_frame = 17; // 1000 / 60 = 16.6(6)
	}
	VSyncCallback(vblank_handler);
	ExitCriticalSection();

	printf("PSX CD init...");
    CdInit();
	printf("OK\n");

	CdControl(CdlNop, 0, 0);
	CdStatus();

	// printf("Files on CD:\n");
	// CdlDIR * dir = CdOpenDir("\\");
	// CdlFILE files[32];
	// int files_found = 0;
	// if (dir) {
	// 	while(CdReadDir(dir, &files[files_found]) ) {
	// 		printf("%s\n", files[files_found].name);
	// 		files_found++;
	// 	}
	// 	CdCloseDir(dir);
	// } else {
	// 	printf("directory not found\n");
	// }

	//  signal(SIGFPE, floating_point_exception_handler);
    // signal(SIGFPE, SIG_IGN);

    memset(&parms, 0, sizeof(parms));

	printf("Com init...");
    COM_InitArgv(c, v);
	printf("OK\n");
    parms.argc = com_argc;
    parms.argv = com_argv;


    parms.memsize = sizeof(membase);
	parms.membase = membase;

    parms.basedir = basedir;
// caching is disabled by default, use -cachedir to enable
//  parms.cachedir = cachedir;

    // fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);

	printf("Host init...");
    Host_Init(&parms);
	printf("OK\n");

	printf("Sys init...");
    Sys_Init();
	printf("OK\n");

	printf("PSX Quake\n");

    oldtime = Sys_MilliTime() - 100;

    while (1) {
		// find time spent rendering last frame
        newtime = Sys_MilliTime();
        time = newtime - oldtime;

        if (time > sys_ticrate.value * 2 * MS_PER_S)
            oldtime = newtime;
        else
            oldtime += time;

        Host_Frame(time);
    }

}
