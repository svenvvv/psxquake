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

#include <psxgpu.h>
#include <psxgte.h>

// viddef_t vid;                // global video state

#define BASEWIDTH   320
#define BASEHEIGHT  240
// #define BASEHEIGHT  200

byte    vid_buffer[BASEWIDTH * BASEHEIGHT];
uint16_t    vid_buffer16[BASEWIDTH * BASEHEIGHT];
short   zbuffer[BASEWIDTH * BASEHEIGHT];
byte    surfcache[256 * 1024];

uint8_t pribuf[2][1024];

unsigned short  d_8to16table[256];
unsigned    d_8to24table[256];

#define OTLEN 8
static uint32_t ot[2][OTLEN];    // Ordering table length

static DISPENV disp[2];
static DRAWENV draw[2];

static uint16_t psx_rgb16(uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t p = 0;

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

void VID_SetPalette(unsigned char * palette)
{
    static int haspalette = 0;
    Sys_Printf("VID_SetPalette\n");

    // TODO palette gets corrupted for some reason with the second call
    if (haspalette) {
        return;
    } else {
        haspalette = 1;
    }

    for (int i = 0; i < 256; ++i) {
        d_8to16table[i] = psx_rgb16(palette[i*3], palette[i*3+1], palette[i*3+2]);
        // TODO PSX unused for now
        // d_8to24table[i] = psx_rgb24(palette[i*3], palette[i*3+1], palette[i*3+2]);
    }
}

void VID_ShiftPalette(unsigned char * palette)
{
    Sys_Printf("VID_ShiftPalette\n");
    VID_SetPalette(palette);
}

void VID_Init(unsigned char * palette)
{
    vid.maxwarpwidth = vid.width = vid.conwidth = BASEWIDTH;
    vid.maxwarpheight = vid.height = vid.conheight = BASEHEIGHT;
    vid.aspect = 1.0;
    vid.numpages = 1;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong(*((int *)vid.colormap + 2048));
    vid.buffer = vid.conbuffer = vid_buffer;
    vid.rowbytes = vid.conrowbytes = BASEWIDTH;

    d_pzbuffer = zbuffer;
    D_InitCaches(surfcache, sizeof(surfcache));

    // SetDefDispEnv(&disp, 0, 0, BASEWIDTH, BASEHEIGHT);
    // SetDefDrawEnv(&draw, 0, 0, BASEWIDTH, BASEHEIGHT);

	SetDefDispEnv(&disp[0], 0, 0, BASEWIDTH, BASEHEIGHT);
    SetDefDrawEnv(&draw[0], 0, BASEHEIGHT, BASEWIDTH, BASEHEIGHT);
    // Second buffer
    SetDefDispEnv(&disp[1], 0, BASEHEIGHT, BASEWIDTH, BASEHEIGHT);
    SetDefDrawEnv(&draw[1], 0, 0, BASEWIDTH, BASEHEIGHT);

    disp[0].isinter = 0; /* Enable interlace (required for hires) */
    disp[1].isinter = 0; /* Enable interlace (required for hires) */

    /* Set clear color, area clear and dither processing */
    setRGB0(&draw[0], 63, 0, 127);
    draw[0].isbg = 1;
    draw[0].dtd = 1;
    setRGB0(&draw[1], 63, 0, 127);
    draw[1].isbg = 1;
    draw[1].dtd = 1;

    /* Apply the display and drawing environments */
    // PutDispEnv(&disp);
    // PutDrawEnv(&draw);

	SetDispMask(1);
}

void VID_Shutdown(void)
{
}

int db = 0;

void VID_Update(vrect_t * rects)
{
	// ClearOTagR(ot[db], OTLEN);  // Clear ordering table

	// unsigned tpage = getTPage(2, 0, BASEWIDTH, BASEHEIGHT);
    draw[db].tpage = getTPage(4, 0, 0, 0);

    TIM_IMAGE tim;

    RECT r = {
        .x = 0,
        .y = 0,
        .w = BASEWIDTH,
        .h = BASEHEIGHT,
    };
    tim.mode = 4;
    tim.prect = &r;
    tim.paddr = (void*)vid_buffer16;

    RECT c = {
        .x = 0,
        .y = 0,
        .w = BASEWIDTH,
        .h = BASEHEIGHT,
    };
    // tim.crect = &c;
    // tim.caddr = (void*)d_8to16table;
    tim.crect = NULL;
    tim.caddr = NULL;

    for (int i = 0; i < sizeof(vid_buffer); ++i) {
        vid_buffer16[i] = d_8to16table[vid_buffer[i]];
    }

    LoadImage(tim.prect, (uint32_t*)tim.paddr);
    // LoadImage(tim.crect, tim.caddr);

    /* Wait for GPU and VSync */
    DrawSync(0);
    VSync(0);

    /* Since draw.isbg is non-zero this clears the screen */
    PutDispEnv(&disp[db]);
    PutDrawEnv(&draw[db]);

	SetDispMask(1);             // Enable the display

    // DrawOTag(ot[db]+OTLEN-1);

	// db = !db;
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

