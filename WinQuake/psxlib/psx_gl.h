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

// 4 pages less than actual since the framebuffer covers the entire four pages, so we can't
// allocate textures on them anyways
#define VRAM_PAGES (((VRAM_WIDTH / VRAM_PAGE_WIDTH) * (VRAM_HEIGHT / VRAM_PAGE_HEIGHT)) - 4)

#define OT_LEN 		(1 * 1024)
#define MENU_OT_LEN (128)
#define PRIBUF_LEN 	(180 * 1024)

struct vram_texture;

#include <psxgte.h>
#include <psxgpu.h>

typedef struct DISPENV_CACHE_S
{
	uint32_t fb_pos;
    uint32_t h_range;
    uint32_t v_range;
    uint32_t mode;
} DISPENV_CACHE;

struct PQRenderBuf
{
	DISPENV disp;
	DISPENV_CACHE disp_cache;
	DRAWENV draw;
	uint32_t ot[OT_LEN];
	uint32_t menu_ot[MENU_OT_LEN];
	uint8_t pribuf[PRIBUF_LEN];
};

struct vram_texpage
{
	/** Page X-coordinate */
	int16_t x;
	/** Page Y-coordinate */
	int16_t y;
	/** Textures allocated on this page */
	struct vram_texture * textures[PSX_MAX_VRAM_RECTS / VRAM_PAGES];
	/** Number of textures allocated on this page */
	size_t textures_count;
	/** Unused rectangles on this page */
	RECT available_rects[PSX_MAX_VRAM_RECTS / VRAM_PAGES];
	/** Number of unused rectangles on this page */
	size_t available_rects_count;
};

struct vram_texture
{
	/** Unique identifier of the texture */
	uint32_t ident;
	/** Texture index. TODO remove? */
	uint16_t index;
	/** Texture rectangle in VRAM */
	RECT rect;
	/** Parent texture page */
    struct vram_texpage * page;
	/** Scale of the output texture in relation to the VRAM texture (we downscale) */
    int scale;
	/** Whether the texture contains transparent pixels */
    bool is_alpha;
	/** PSX texture page value */
	uint16_t tpage;
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
void psx_vram_rect(int x, int y, int w, int h);

void psx_rb_init(void);
void psx_rb_present(void);

// void psx_add_prim(void * prim, int z);

// static inline void psx_add_prim(void * prim, int z)
// {
//     extern int pricount;
//     extern struct PQRenderBuf rb[2];
//
// 	pricount += 1;
// 	if (z < 0) {
// 		z += OT_LEN;
// 	}
// 	addPrim(rb[psx_db].ot + (OT_LEN - z - 1), prim);
// 	// rb_nextpri = prim;
// }

template <typename T>
static inline void psx_add_prim(T * prim, int z)
{
    extern int pricount;
    extern struct PQRenderBuf rb[2];

	pricount += 1;
	if (z < 0) {
		z += OT_LEN;
	}

	addPrim(rb[psx_db].ot + (OT_LEN - z - 1), prim);
	rb_nextpri = (uint8_t *) ++prim;
}

template <typename T>
static inline void menu_add_prim_z(T * prim, int z)
{
    extern int pricount;
    extern struct PQRenderBuf rb[2];

	pricount += 1;

	addPrim(rb[psx_db].menu_ot + (MENU_OT_LEN - z - 1), prim);
	rb_nextpri = (uint8_t *) ++prim;
}

template <typename T>
static inline void psx_add_prim_z(T * prim, int z)
{
    extern int pricount;
    extern struct PQRenderBuf rb[2];

	// if (pricount > 1000) {
	// 	return;
	// }

	pricount += 1;

	addPrim(rb[psx_db].ot + z, prim);
	rb_nextpri = (uint8_t *) prim + sizeof(T);
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

template <typename T>
T * psx_nextpri()
{
	return (T*)rb_nextpri;
}

static constexpr size_t PSX_SCRATCH_SIZE = 1024;
static uint8_t * psx_scratch = (uint8_t*) 0x1F800000;

void draw_quad_tex_subdiv(SVECTOR const verts[4], SVECTOR const & normal,
						  uint8_t const uv[4 * 2], struct vram_texture const * tex,
						  int subdiv);

void draw_tri_tex_subdiv(SVECTOR const verts[3], SVECTOR const & normal,
						 uint8_t const uv[3 * 2], struct vram_texture const * tex,
						 int subdiv);

void draw_tri(SVECTOR const verts[3], CVECTOR const * color);
void draw_tri_tex(SVECTOR const verts[3], SVECTOR const & normal,
				  uint8_t const uv[3 * 2], struct vram_texture const * tex);
void draw_quad(SVECTOR const verts[4], CVECTOR const * color);
int draw_quad_tex(SVECTOR const verts[4], SVECTOR const & normal,
				  uint8_t const uv[4 * 2], struct vram_texture const * tex);
