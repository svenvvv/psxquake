#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <psxgpu.h>

#define PSX_MAX_VRAM_RECTS 2048

#define VID_WIDTH 				320 // can't lower it without menus breaking
#define VID_HEIGHT 				240

#define VRAM_WIDTH 1024
#define VRAM_HEIGHT 512
#define VRAM_PAGE_WIDTH 128
#define VRAM_PAGE_HEIGHT 256
#define VRAM_PAGES ((VRAM_WIDTH / VRAM_PAGE_WIDTH) * (VRAM_HEIGHT / VRAM_PAGE_HEIGHT))

#define OT_LEN 		1024
#define PRIBUF_LEN 	(32 * 1024)

struct PQRenderBuf
{
	DISPENV disp;
	DRAWENV draw;
	uint32_t ot[OT_LEN];
	uint8_t pribuf[PRIBUF_LEN];
};

struct vram_texpage
{
	RECT rect;
	struct vram_texture * textures[PSX_MAX_VRAM_RECTS / VRAM_PAGES];
	size_t textures_count;
	RECT available_rects[PSX_MAX_VRAM_RECTS / VRAM_PAGES];
	size_t available_rects_count;
};

struct vram_texture
{
	uint32_t ident;
	uint16_t index;
	RECT rect;
    int div;
    struct vram_texpage * page;
    bool alpha;
	uint16_t tpage;
    bool permanent;
};

extern uint8_t * rb_nextpri;

extern uint16_t psx_clut;
extern uint16_t psx_clut_transparent;
extern uint16_t psx_zlevel;
extern int psx_db;

void psx_vram_init (void);
struct vram_texture * psx_vram_get (int index);
struct vram_texture * psx_vram_pack (char * ident, int w, int h);
struct vram_texture * psx_vram_find (char * ident, int w, int h);

void psx_rb_init(void);
void psx_rb_present(void);

// void psx_add_prim(void * prim, int z);

static inline void psx_add_prim(void * prim, int z)
{
    extern int pricount;
    extern struct PQRenderBuf rb[2];

	pricount += 1;
	if (z < 0) {
		z += OT_LEN;
	}
	addPrim(rb[psx_db].ot + (OT_LEN - z - 1), prim);
	// rb_nextpri = prim;
}

static inline uint16_t psx_rgb16(uint8_t r, uint8_t g, uint8_t b, uint8_t stp)
{
    uint16_t p = (stp & 1) << 15;

    p |= (r >> 3) << 0;
    p |= (g >> 3) << 5;
    p |= (b >> 3) << 10;

    return p;
}

static inline uint32_t psx_rgb24(uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t ret = 0;

    ret |= r << 0;
    ret |= g << 16;
    ret |= b << 24;

    return ret;
}
