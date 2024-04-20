/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"
#include <stdbool.h>

#include <psxgpu.h>
#include <psxgte.h>
#include <inline_c.h>

#define WARP_WIDTH              256
#define WARP_HEIGHT             256

#define VID_WIDTH 				320 // can't lower it without menus breaking
#define VID_HEIGHT 				240

#define stringify(m) { #m, m }

unsigned short	d_8to16table[256];
unsigned	d_8to24table[256]; //TODO this can be removed for PSX GL
// unsigned char d_15to8table[65536];

cvar_t		vid_mode = {"vid_mode","5",false};
cvar_t		vid_redrawfull = {"vid_redrawfull","0",false};
cvar_t		vid_waitforrefresh = {"vid_waitforrefresh","0",true};
 
cvar_t	m_filter = {"m_filter","1"};


/*-----------------------------------------------------------------------*/

//int		texture_mode = GL_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_LINEAR;
int		texture_mode = GL_LINEAR;
//int		texture_mode = GL_LINEAR_MIPMAP_NEAREST;
//int		texture_mode = GL_LINEAR_MIPMAP_LINEAR;

int		texture_extension_number = 1;

float		gldepthmin, gldepthmax;

cvar_t	gl_ztrick = {"gl_ztrick","1"};

const char *gl_vendor;
const char *gl_renderer;
const char *gl_version;
const char *gl_extensions;

qboolean is8bit = true;
qboolean isPermedia = false;
qboolean gl_mtexable = false;


#define OT_LEN 		256
#define PRIBUF_LEN 	(8 * 1024)

struct PQRenderBuf
{
	DISPENV disp;
	DRAWENV draw;
	uint32_t ot[OT_LEN];
	uint8_t pribuf[PRIBUF_LEN];
};

static int db = 0;
static struct PQRenderBuf rb[2];
uint8_t * rb_nextpri;
uint16_t psx_zlevel = 0;;

void psx_add_prim(void * prim, int z)
{
	if (z < 0) {
		z += OT_LEN;
	}
	addPrim(rb[db].ot + (OT_LEN - z - 1), prim);
	// rb_nextpri = prim;
}

static inline uint16_t psx_rgb16(uint16_t init, uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t p = init;

    p |= (r >> 3) << 0;
    p |= (g >> 3) << 5;
    p |= (b >> 3) << 10;

    return p;
}

static uint32_t psx_rgb24(uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t ret = 0;

    ret |= r << 0;
    ret |= g << 16;
    ret |= b << 24;

    return ret;
}


/*-----------------------------------------------------------------------*/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}

void D_EndDirectRect (int x, int y, int width, int height)
{
}

void VID_Shutdown(void)
{
}

void VID_ShiftPalette(unsigned char *p)
{
	VID_SetPalette(p);
}

static RECT psx_clut_rect = {
	.x = 0,
	.y = (2 * VID_HEIGHT),
	.w = 16 * 16,
	.h = 1// 16,
};

static RECT psx_clut_transparent_rect = {
	.x = 0,
	.y = (2 * VID_HEIGHT) + 1,
	.w = 16 * 16,
	.h = 1// 16,
};

uint16_t psx_clut = 0;
uint16_t psx_clut_transparent = 0;

void	VID_SetPalette (unsigned char *palette)
{
    for (int i = 0; i < 256; ++i) {
        d_8to16table[i] = psx_rgb16(0x8000, palette[i*3], palette[i*3+1], palette[i*3+2]);
        // d_8to24table[i] = psx_rgb24(palette[i*3], palette[i*3+1], palette[i*3+2]);
    }
	LoadImage(&psx_clut_rect, (uint32_t*) d_8to16table);
	psx_clut = getClut(psx_clut_rect.x, psx_clut_rect.y);

    for (int i = 0; i < 256; ++i) {
        d_8to16table[i] = psx_rgb16(0, palette[i*3], palette[i*3+1], palette[i*3+2]);
    }
	LoadImage(&psx_clut_transparent_rect, (uint32_t*) d_8to16table);
	psx_clut_transparent = getClut(psx_clut_transparent_rect.x, psx_clut_transparent_rect.y);
}

/*
===============
GL_Init
===============
*/

static MATRIX color_mtx = {
	ONE * 3/4, 0, 0,	/* Red   */
	ONE * 3/4, 0, 0,	/* Green */
	ONE * 3/4, 0, 0	/* Blue  */
};

void GL_Init (void)
{
	ResetGraph(0);

	SetDefDispEnv(&rb[0].disp, 0, 0, VID_WIDTH, VID_HEIGHT);
	SetDefDrawEnv(&rb[0].draw, 0, VID_HEIGHT, VID_WIDTH, VID_HEIGHT);
    // Set clear color, area clear and dither processing
    setRGB0(&rb[0].draw, 63, 0, 127);
    rb[0].draw.isbg = 1;
    rb[0].draw.dtd = 1;

	SetDefDispEnv(&rb[1].disp, 0, VID_HEIGHT, VID_WIDTH, VID_HEIGHT);
	SetDefDrawEnv(&rb[1].draw, 0, 0, VID_WIDTH, VID_HEIGHT);
    // Set clear color, area clear and dither processing
    setRGB0(&rb[1].draw, 63, 0, 127);
    rb[1].draw.isbg = 1;
    rb[1].draw.dtd = 1;

	PutDispEnv(&rb[0].disp);
	PutDrawEnv(&rb[0].draw);

	ClearOTagR(rb[0].ot, ARRAY_SIZE(rb->ot));
	ClearOTagR(rb[1].ot, ARRAY_SIZE(rb->ot));

	rb_nextpri = rb[0].pribuf;

	InitGeom();

	gte_SetGeomOffset(VID_WIDTH >> 1, VID_HEIGHT >> 1);
	gte_SetGeomScreen(VID_WIDTH >> 1);
	gte_SetBackColor(63, 63, 63);
	gte_SetColorMatrix(&color_mtx);

	// SetDispMask(1);
	SetDispMask(1);
}

/*
=================
GL_BeginRendering

=================
*/
void GL_BeginRendering (int *x, int *y, int *width, int *height)
{

}

void GL_EndRendering (void)
{
    DrawSync(0);
    VSync(0);

	db = !db;
	rb_nextpri = rb[db].pribuf;

	ClearOTagR(rb[db].ot, ARRAY_SIZE(rb->ot));

	PutDispEnv(&rb[db].disp);
	PutDrawEnv(&rb[db].draw);

	SetDispMask(1);

	DrawOTag(rb[!db].ot + (OT_LEN - 1));
	if (psx_zlevel >= OT_LEN) {
		Sys_Error("psx_zlevel out of bounds\n");
	}
	psx_zlevel = 0;
}

qboolean VID_Is8bit(void)
{
	return is8bit;
}

static struct vram_texture vram_textures[PSX_MAX_VRAM_RECTS];
static size_t vram_textures_count = 0;

static struct vram_texpage vram_pages[VRAM_PAGES];

static struct vram_texture * vram_tex_alloc(void)
{
	int id;
	struct vram_texture * ret;

	EnterCriticalSection();
	id = vram_textures_count++;
	ExitCriticalSection();
	if (id >= ARRAY_SIZE(vram_textures)) {
		Sys_Error("Out of vram_textures\n");
	}

	vram_textures[id].index = id;
	return &vram_textures[id];
}

struct vram_texture * psx_vram_get (int index)
{
	if (vram_textures_count <= index) {
		return NULL;
	}

	return &vram_textures[index];
}

struct vram_texture * psx_vram_find (char * ident, int w, int h)
{
	uint32_t ident_hash = 0;

	if (!ident[0]) {
		return NULL;
	}

	ident_hash = crc32(ident, strlen(ident));

	for (int i = 0; i < vram_textures_count; ++i) {
		struct vram_texture * tex = &vram_textures[i];
		if (tex->ident == ident_hash) {
			if (w > 0 && w != tex->rect.w || h > 0 && h != tex->rect.h) {
 				Sys_Error ("psx_vram_find: cache mismatch 0x%X: %ix%i != %ix%i\n",
						   tex->ident, w, h, tex->rect.w, tex->rect.h);
			}
			return tex;
		}
	}

	return NULL;
}

void psx_vram_rect(int x, int y, int w, int h)
{
	return;
	GL_BeginRendering(0, 0, 0, 0);
	FILL * fill = (FILL*)rb_nextpri;
	setFill(fill);
	setXY0(fill, x, y);
	setWH(fill, w, h);
	setRGB0(fill, rand() % 0xFF, rand() % 0xFF, rand() % 0xFF);
	psx_add_prim(fill, 0);
	rb_nextpri = (void*)++fill;
	GL_EndRendering();
}

void psx_vram_compact(struct vram_texpage * page)
{
	int largest_w = 0;
	RECT * available_rect;

	for (int t = 0; t < page->textures_count; ++t) {
		struct vram_texture * tex = page->textures[t];
		int w = tex->rect.x + tex->rect.w / 2;
		if (w > largest_w) {
			largest_w = w;
		}
	}

	if (largest_w == VRAM_PAGE_WIDTH) {
		printf("Unable to recompact page\n");
		return;
	}

	printf("page largest w %d\n", largest_w);

	EnterCriticalSection();
	available_rect = &page->available_rects[page->available_rects_count++];
	ExitCriticalSection();
	if (page->available_rects_count >= ARRAY_SIZE(page->available_rects)) {
		Sys_Error("No more available_rects in page\n");
	}

	// TODO remove largest_w from previous available rects

	available_rect->x = largest_w;
	available_rect->y = 0;
	available_rect->w = VRAM_PAGE_WIDTH - largest_w;
	available_rect->h = VRAM_PAGE_HEIGHT;

	psx_vram_rect(
		available_rect->x + page->rect.x,
		available_rect->y + page->rect.y,
		available_rect->w,
		available_rect->h
	);
}

struct vram_texture * psx_vram_pack (char * ident, int w, int h)
{
	uint32_t ident_hash = 0;
	struct vram_texture * target_tex = NULL;

	if (ident[0]) {
		ident_hash = crc32(ident, strlen(ident));
	}

	w /= 2;

	for (int p = 0; p < ARRAY_SIZE(vram_pages); ++p) {
 		struct vram_texpage * page = &vram_pages[p];

		if (page->available_rects_count == 0) {
			continue;
		}

		for (int r = 0; r < page->available_rects_count; ++r) {
			RECT * available_rect = &page->available_rects[r];
			int remaining_w = available_rect->w - w;
			int remaining_h = available_rect->h - h;

			if (remaining_w < 0 || remaining_h < 0) {
				continue;
			}

			target_tex = vram_tex_alloc();
			target_tex->ident = ident_hash;
			target_tex->rect.x = available_rect->x;
			target_tex->rect.y = available_rect->y;
			target_tex->rect.w = w * 2;
			target_tex->rect.h = h;
			target_tex->page = page;
			target_tex->tpage = getTPage(1, 0,
										page->rect.x + target_tex->rect.x,
										page->rect.y + target_tex->rect.y);

			EnterCriticalSection();
 			page->textures[page->textures_count++] = target_tex;
			ExitCriticalSection();
			if (page->textures_count >= ARRAY_SIZE(page->textures)) {
				Sys_Error("No more textures in page\n");
			}

			if (remaining_w == 0 || remaining_h == 0) {
				printf("Discarding %i;%i;%i;%i, remaining %ix%i\n",
					   available_rect->x, available_rect->y,
					   available_rect->w, available_rect->h,
					   remaining_w, remaining_h);
				// Discard if small
				memmove(page->available_rects + r, page->available_rects + r + 1,
						sizeof(*available_rect) * (page->available_rects_count - r));
				page->available_rects_count -= 1;
			} else {
				// Split available rect around the newly allocated texture
				printf("Splitting %i;%i;%i;%i...\n",
					   available_rect->x, available_rect->y,
					   available_rect->w, available_rect->h);

				// Right side left-overs
				if (remaining_w > 0) {
					available_rect->x += w;
					available_rect->w -= w;
					available_rect->h = h;
					printf("...right %i;%i;%i;%i\n",
						available_rect->x, available_rect->y,
						available_rect->w, available_rect->h);

					psx_vram_rect(
						available_rect->x + page->rect.x,
						available_rect->y + page->rect.y,
						available_rect->w,
						available_rect->h
					);
				}

				// Bottom left-overs
				if (remaining_h > 0) {
					if (remaining_w > 0) {
						// Allocate new rect for bottom
						EnterCriticalSection();
						available_rect = &page->available_rects[page->available_rects_count++];
						ExitCriticalSection();
						if (page->available_rects_count >= ARRAY_SIZE(page->available_rects)) {
							Sys_Error("No more available_rects in page\n");
						}
					}
					available_rect->x = target_tex->rect.x;
					available_rect->y = target_tex->rect.y + h;
					available_rect->w = remaining_w + w;
					available_rect->h = remaining_h;
					printf("...bottom %i;%i;%i;%i\n",
						available_rect->x, available_rect->y,
						available_rect->w, available_rect->h);
					psx_vram_rect(
						available_rect->x + page->rect.x,
						available_rect->y + page->rect.y,
						available_rect->w,
						available_rect->h
					);
				}
			}

			goto exit;
		}
	}

	if (target_tex == NULL) {
		for (int p = 0; p < ARRAY_SIZE(vram_pages); ++p) {
			// TODO PSX lazy
			if (p == 0 || p == 1 || p == 4 || p == 5) continue;
			printf("Unable to fit new texture, recompacting page %d...\n", p);
			struct vram_texpage * page = &vram_pages[p];
			psx_vram_compact(page);
		}
	}

exit:
	return target_tex;
}

// struct vram_texture * psx_vram_pack (char * ident, int w, int h)
// {
// 	uint16_t ident_hash = 0;
//
// 	if (ident[0]) {
// 		ident_hash = crc32(ident, strlen(ident));
// 	}
//
// 	for (int p = 0; p < ARRAY_SIZE(vram_pages); ++p) {
// 		struct vram_texpage * page = &vram_pages[p];
// 		struct vram_texture * target_tex = NULL;
// 		int xoff = page->rect.x;
// 		int yoff = page->rect.y;
// 		int arr_pos = -1;
//
// 		// printf("Page %i, texc %i\n", p, page->textures_count);
//
// 		if (page->is_full) {
// 			// Page doesn't have room
// 			continue;
// 		}
//
// 		for (int t = 0; t < page->textures_count; ++t) {
// 			struct vram_texture * tex = page->textures[t];
// 			struct vram_texture * tex_next = NULL;
//
// 			// printf("search %i = %i %i, %p\n", t, h, tex->rect.h, tex);
//
// 			// If tex doesn't fit in the same row then skip
// 			if (h > tex->rect.h) {
// 				continue;
// 			}
//
// 			int free_w = VRAM_PAGE_WIDTH - tex->rect.w;
// 			// printf("free w 1 %i\n", free_w);
//
// 			for (int n = t + 1; n < page->textures_count; ++n) {
// 				// printf("free w 2 %i %i, %i %i\n", page->textures[n]->rect.y, tex->rect.y, free_w, w);
// 				if (page->textures[n]->rect.y != tex->rect.y) {
// 					arr_pos = n;
// 					break;
// 				}
//
// 				free_w -= tex_next->rect.w;
//
// 				if (free_w < w) {
// 					tex_next = NULL;
// 					break;
// 				}
// 				tex_next = page->textures[n];
// 			}
//
// 			if (free_w < w) {
// 				continue;
// 			}
//
// 			if (tex_next) {
// 				xoff = tex_next->rect.x + tex_next->rect.w;
// 				yoff = tex_next->rect.y;
// 			} else {
// 				xoff = tex->rect.x + tex->rect.w;
// 				yoff = tex->rect.y;
// 				arr_pos = t + 1;
// 			}
//
// 			printf("Place next to %i, %i;%i, free width %i\n", arr_pos, xoff, yoff, free_w);
// 			goto place_next;
// 		}
//
// 		// If no room found on the same row as existing then try allocating a new row
// 		if (VRAM_PAGE_HEIGHT - page->used_h < h) {
// 			continue;
// 		}
//
// 		yoff += page->used_h;
//
// place_next:
// 		target_tex = vram_tex_alloc();
// 		target_tex->ident = ident_hash;
// 		target_tex->rect.x = xoff;
// 		target_tex->rect.y = yoff;
// 		target_tex->rect.w = w;
// 		target_tex->rect.h = h;
//
// 		page->used_h += h;
//
// 		printf("pack target %i;%i %ix%i\n",
// 			   target_tex->rect.x, target_tex->rect.y, target_tex->rect.w, target_tex->rect.h);
// 		if (arr_pos + 1 < page->textures_count) {
// 			printf("pack move %p %p, count %u\n", page->textures + arr_pos + 1, page->textures + arr_pos, page->textures_count - arr_pos);
// 			memmove(page->textures + arr_pos + 1, page->textures + arr_pos, (page->textures_count - arr_pos) * sizeof(target_tex));
// 			page->textures[arr_pos] = target_tex;
// 			page->textures_count += 1;
// 		} else {
// 			page->textures[page->textures_count++] = target_tex;
// 		}
//
// 		if (page->textures_count == ARRAY_SIZE(page->textures)) {
// 			page->is_full = true;
// 		}
//
// 		return target_tex;
// 	}
//
// 	return NULL;
// }

void psx_vram_init (void)
{
	for (int p = 0; p < ARRAY_SIZE(vram_pages); ++p) {
		struct vram_texpage * page = &vram_pages[p];
		page->rect.x = p * VRAM_PAGE_WIDTH;
		if (page->rect.x >= VRAM_WIDTH) {
			page->rect.x -= VRAM_WIDTH;
			page->rect.y = VRAM_PAGE_HEIGHT;
		} else {
			page->rect.y = 0;
		}
		page->rect.w = VRAM_PAGE_WIDTH;
		page->rect.h = VRAM_PAGE_HEIGHT;
		page->textures_count = 0;
		// page->used_h = 0;
		// page->is_full = false;
		printf("TexPage init %i;%i %ix%i\n",
			   page->rect.x, page->rect.y, page->rect.w, page->rect.h);

		page->available_rects[0].x = 0;
		page->available_rects[0].y = 0;
		page->available_rects[0].w = VRAM_PAGE_WIDTH;
		page->available_rects[0].h = VRAM_PAGE_HEIGHT;
		page->available_rects_count = 1;
	}

	int vidpartw = VID_WIDTH - VRAM_PAGE_WIDTH;
	vram_pages[0].available_rects_count = 0;
	// vram_pages[1].available_rects_count = 0;
	vram_pages[1].available_rects[0].x = vidpartw;
	vram_pages[1].available_rects[0].w -= vidpartw;
	int texid = vram_textures_count++;
	vram_pages[1].textures[0] = &vram_textures[texid];
	vram_pages[1].textures[0]->ident = 0;
	vram_pages[1].textures[0]->index = texid;
	vram_pages[1].textures[0]->rect.x = 0;
	vram_pages[1].textures[0]->rect.y = 0;
	vram_pages[1].textures[0]->rect.w = vidpartw;
	vram_pages[1].textures[0]->rect.h = VID_HEIGHT;

	vram_pages[4].available_rects_count = 0;
	// vram_pages[5].available_rects_count = 0;
	vram_pages[5].available_rects[0].x = vidpartw;
	vram_pages[5].available_rects[0].w -= vidpartw;
	texid = vram_textures_count++;
	vram_pages[5].textures[0] = &vram_textures[texid];
	vram_pages[5].textures[0]->ident = 0;
	vram_pages[5].textures[0]->index = texid;
	vram_pages[5].textures[0]->rect.x = 0;
	vram_pages[5].textures[0]->rect.y = 0;
	vram_pages[5].textures[0]->rect.w = vidpartw;
	vram_pages[5].textures[0]->rect.h = VID_HEIGHT;

	// vram_pages[1].available_rects[0].x = VID_WIDTH - VRAM_PAGE_WIDTH;
	// vram_pages[1].available_rects[0].w -= VID_WIDTH - VRAM_PAGE_WIDTH;
	// vram_pages[5].available_rects[0].x = VID_WIDTH - VRAM_PAGE_WIDTH;
	// vram_pages[5].available_rects[0].w -= VID_WIDTH - VRAM_PAGE_WIDTH;

	// vram_pages[0].is_full = true;
	// vram_pages[1].is_full = true;
	// vram_pages[5].is_full = true;
	// vram_pages[6].is_full = true;
}

void VID_Init(unsigned char *palette)
{
	psx_vram_init();

	Cvar_RegisterVariable (&vid_mode);
	Cvar_RegisterVariable (&vid_redrawfull);
	Cvar_RegisterVariable (&vid_waitforrefresh);
	Cvar_RegisterVariable (&gl_ztrick);

	vid.maxwarpwidth = WARP_WIDTH;
	vid.maxwarpheight = WARP_HEIGHT;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));

	vid.conwidth = VRAM_PAGE_WIDTH;
	vid.conheight = VRAM_PAGE_HEIGHT; //vid.conwidth*3 / 4;

	// pick a conheight that matches with correct aspect
	// vid.conheight = vid.conwidth*3 / 4;
	// vid.conheight = 200;

	vid.width = VID_WIDTH;
	vid.height = VID_HEIGHT;

	// vid.width = vid.conwidth;
	// vid.height = vid.conheight;

	vid.aspect = ((float)vid.height / (float)vid.width) *
				(320.0 / 240.0);
	vid.numpages = 2;

	GL_Init();

	// Check_Gamma(palette);
	VID_SetPalette(palette);

	vid.recalc_refdef = 1;				// force a surface cache flush

	// Clear VRAM for debugging
	GL_BeginRendering(0, 0, 0, 0);
	for (int y = 0; y < VRAM_HEIGHT; y += 256) {
		for (int x = 0; x < VRAM_WIDTH; x += 256) {
			FILL * fill = (FILL*)rb_nextpri;
			setFill(fill);
			setXY0(fill, x, y);
			setWH(fill, 256, 256);
			setRGB0(fill, 0, 0xFF, 0);
			psx_add_prim(fill, 0);
			rb_nextpri = (void*)++fill;
		}
	}
	GL_EndRendering();
}

void Sys_SendKeyEvents(void)
{
}

void Force_CenterView_f (void)
{
	cl.viewangles[PITCH] = 0;
}

// void IN_Init(void)
// {
// }
//
// void IN_Shutdown(void)
// {
// }
//
// /*
// ===========
// IN_Commands
// ===========
// */
// void IN_Commands (void)
// {
// }
//
// /*
// ===========
// IN_Move
// ===========
// */
// void IN_MouseMove (usercmd_t *cmd)
// {
// }
//
// void IN_Move (usercmd_t *cmd)
// {
// 	IN_MouseMove(cmd);
// }


