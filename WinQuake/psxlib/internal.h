#pragma once

#include "psx_io.h"

#define PSX_CD_SECTOR_SIZE 2048
#define PSX_CD_FILES_MAX 4

struct PQCdFile
{
	int allocated;
	uint16_t filename_hash;
	// Current read cursor, in CD-drive sectors
	size_t cursor_sectors;
	// Current read cursor offset, in bytes
	size_t cursor_bytes;
	CdlFILE file;
    int read_buf_is_valid;
    uint8_t read_buf[PSX_CD_SECTOR_SIZE];
};

extern struct PQCdFile pq_cd_files[PSX_CD_FILES_MAX];

struct PQCdFile * pq_cd_file_alloc(void);

int pq_cd_file_is_valid(struct PQCdFile const * f);
