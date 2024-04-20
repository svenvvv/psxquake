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
// vid_null.c -- null video driver to aid porting efforts

#include "quakedef.h"
#include "d_local.h"
#include "psxlib/psx_gl.h"

#include <psxgpu.h>
#include <psxgte.h>

// viddef_t vid;                // global video state

#define VID_WIDTH   320
#define VID_HEIGHT  240

byte    vid_buffer[VID_WIDTH * VID_HEIGHT];
short   zbuffer[VID_WIDTH * VID_HEIGHT];
byte    surfcache[256 * 1024];

unsigned short  d_8to16table[256];
unsigned    d_8to24table[256];

static RECT psx_clut_rect = {
	.x = 0,
	.y = (2 * VID_HEIGHT),
	.w = 16 * 16,
	.h = 1,
};
uint16_t psx_clut = 0;

void VID_SetPalette(unsigned char * palette)
{
    for (int i = 0; i < 256; ++i) {
        d_8to16table[i] = psx_rgb16(palette[i*3], palette[i*3+1], palette[i*3+2], 0);
        d_8to24table[i] = psx_rgb24(palette[i*3], palette[i*3+1], palette[i*3+2]);
    }
	LoadImage(&psx_clut_rect, (uint32_t*) d_8to16table);
	psx_clut = getClut(psx_clut_rect.x, psx_clut_rect.y);
}

void VID_ShiftPalette(unsigned char * palette)
{
    VID_SetPalette(palette);
}

void VID_Init(unsigned char * palette)
{
    vid.maxwarpwidth = vid.width = vid.conwidth = VID_WIDTH;
    vid.maxwarpheight = vid.height = vid.conheight = VID_HEIGHT;
    vid.aspect = 1.0;
    vid.numpages = 1;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong(*((int *)vid.colormap + 2048));
    vid.buffer = vid.conbuffer = vid_buffer;
    vid.rowbytes = vid.conrowbytes = VID_WIDTH;

    d_pzbuffer = zbuffer;
    D_InitCaches(surfcache, sizeof(surfcache));

    psx_rb_init();
}

void VID_Shutdown(void)
{
}

int db = 0;

void VID_Update(vrect_t * rects)
{
    RECT c = {
        .x = 512,
        .y = psx_db * VID_HEIGHT,
        .w = VID_WIDTH / 2,
        .h = VID_HEIGHT,
    };
    LoadImage(&c, (uint32_t*)vid_buffer);

    POLY_FT4 * p = (void*)rb_nextpri;

    setPolyFT4(p);
    setXYWH(p, 0, 0, 255, VID_HEIGHT);
    setUVWH(p, 0, 0, 255, VID_HEIGHT);
    setRGB0(p, 128, 128, 128);
    setTPage(p, 1, 0, 512, 0);
    p->clut = psx_clut;

    psx_add_prim(p, 0);
    rb_nextpri = (void*)++p;

    p = (void*)rb_nextpri;

    setPolyFT4(p);
    setXYWH(p, 255, 0, VID_WIDTH - 255, VID_HEIGHT);
    setUVWH(p, 0, 0, VID_WIDTH - 255, VID_HEIGHT);
    setRGB0(p, 128, 128, 128);
    setTPage(p, 1, 0, 512 + 128, 0);
    p->clut = psx_clut;

    psx_add_prim(p, 0);
    rb_nextpri = (void*)++p;

    psx_rb_present();
}

void Sys_SendKeyEvents(void)
{
}

/*
================
D_BeginDirectRect
================
*/
void D_BeginDirectRect(int x, int y, byte * pbitmap, int width, int height)
{
	Sys_Printf("VID begin direct rect %ux%u %ux%u\n", x, y, width, height);
}

/*
================
D_EndDirectRect
================
*/
void D_EndDirectRect(int x, int y, int width, int height)
{
	Sys_Printf("VID end direct rect %ux%u %ux%u\n", x, y, width, height);
}

