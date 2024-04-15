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

#define WARP_WIDTH              320
#define WARP_HEIGHT             200

#define stringify(m) { #m, m }

unsigned short	d_8to16table[256];
unsigned	d_8to24table[256];
unsigned char d_15to8table[65536];

int num_shades=32;

int	d_con_indirect = 0;

int		svgalib_inited=0;
int		UseMouse = 1;
int		UseKeyboard = 1;

cvar_t		vid_mode = {"vid_mode","5",false};
cvar_t		vid_redrawfull = {"vid_redrawfull","0",false};
cvar_t		vid_waitforrefresh = {"vid_waitforrefresh","0",true};
 
char	*framebuffer_ptr;

cvar_t	m_filter = {"m_filter","1"};

int scr_width, scr_height;

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

qboolean is8bit = false;
qboolean isPermedia = false;
qboolean gl_mtexable = false;

/*-----------------------------------------------------------------------*/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}

void D_EndDirectRect (int x, int y, int width, int height)
{
}

void keyhandler(int scancode, int state)
{
}

void VID_Shutdown(void)
{
}

void VID_ShiftPalette(unsigned char *p)
{
//	VID_SetPalette(p);
}

void	VID_SetPalette (unsigned char *palette)
{
}

void CheckMultiTextureExtensions(void)
{
}

/*
===============
GL_Init
===============
*/
void GL_Init (void)
{
}

/*
=================
GL_BeginRendering

=================
*/
void GL_BeginRendering (int *x, int *y, int *width, int *height)
{
	extern cvar_t gl_clear;

	*x = *y = 0;
	*width = scr_width;
	*height = scr_height;

//    if (!wglMakeCurrent( maindc, baseRC ))
//		Sys_Error ("wglMakeCurrent failed");

//	glViewport (*x, *y, *width, *height);
}


void GL_EndRendering (void)
{
}

void Init_KBD(void)
{
}

#define NUM_RESOLUTIONS 16

static int resolutions[NUM_RESOLUTIONS][3]={
};

int findres(int *width, int *height)
{
}

qboolean VID_Is8bit(void)
{
	return is8bit;
}

void VID_Init8bitPalette(void)
{
}

static void Check_Gamma (unsigned char *pal)
{
}

void VID_Init(unsigned char *palette)
{
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


