#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <psxgpu.h>

#define PSX_MAX_VRAM_RECTS 2048

#define VRAM_WIDTH 1024
#define VRAM_HEIGHT 512
#define VRAM_PAGE_WIDTH 256
#define VRAM_PAGE_HEIGHT 256
#define VRAM_PAGES ((VRAM_WIDTH / VRAM_PAGE_WIDTH) * (VRAM_HEIGHT / VRAM_PAGE_HEIGHT))

// struct vram_texpage
// {
// 	RECT rect;
// 	bool is_full;
// 	size_t used_h;
// 	struct vram_texture * textures[PSX_MAX_VRAM_RECTS / VRAM_PAGES];
// 	size_t textures_count;
// };
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
};

extern uint8_t * rb_nextpri;

extern uint16_t psx_clut;
extern uint16_t psx_clut_transparent;
extern uint16_t psx_zlevel;

void psx_add_prim(void * prim, int z);

struct vram_texture * psx_vram_get (int index);
struct vram_texture * psx_vram_pack (char * ident, int w, int h);
struct vram_texture * psx_vram_find (char * ident, int w, int h);

