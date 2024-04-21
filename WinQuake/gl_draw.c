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

// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include "quakedef.h"

#define GL_COLOR_INDEX8_EXT     0x80E5

// extern unsigned char d_15to8table[65536];

cvar_t		gl_nobind = {"gl_nobind", "0"};
cvar_t		gl_max_size = {"gl_max_size", "256"}; // PSX texture page limit
cvar_t		gl_picmip = {"gl_picmip", "0"};

byte		*draw_chars;				// 8*8 graphic characters
qpic_t		*draw_disc;
qpic_t		*draw_backtile;

int			translate_texture;
int			char_texture;

typedef struct
{
	int		texnum;
} glpic_t;

byte		conback_buffer[sizeof(qpic_t) + sizeof(glpic_t)];
qpic_t		*conback = (qpic_t *)&conback_buffer;

int		gl_lightmap_format = 4;
int		gl_solid_format = 3;
int		gl_alpha_format = 4;

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;


int		texels;

typedef struct
{
	int		texnum;
	char	identifier[64];
	int		width, height;
	qboolean	mipmap;
} gltexture_t;

void GL_Bind (int texnum)
{
// 	if (gl_nobind.value)
// 		texnum = char_texture;
// 	if (currenttexture == texnum)
// 		return;
// 	currenttexture = texnum;
// #ifdef _WIN32
// 	bindTexFunc (GL_TEXTURE_2D, texnum);
// #else
// 	glBindTexture(GL_TEXTURE_2D, texnum);
// #endif
}


/*
=============================================================================

  scrap allocation

  Allocate all the little status bar obejcts into a single texture
  to crutch up stupid hardware / drivers

=============================================================================
*/

// #define	MAX_SCRAPS		2
// #define	BLOCK_WIDTH		256
// #define	BLOCK_HEIGHT	256
//
// int			scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
// byte		scrap_texels[MAX_SCRAPS][BLOCK_WIDTH*BLOCK_HEIGHT*4];
// qboolean	scrap_dirty;
// int			scrap_texnum;
//
// // returns a texture number and the position inside it
// int Scrap_AllocBlock (int w, int h, int *x, int *y)
// {
// 	int		i, j;
// 	int		best, best2;
// 	int		bestx;
// 	int		texnum;
//
// 	for (texnum=0 ; texnum<MAX_SCRAPS ; texnum++)
// 	{
// 		best = BLOCK_HEIGHT;
//
// 		for (i=0 ; i<BLOCK_WIDTH-w ; i++)
// 		{
// 			best2 = 0;
//
// 			for (j=0 ; j<w ; j++)
// 			{
// 				if (scrap_allocated[texnum][i+j] >= best)
// 					break;
// 				if (scrap_allocated[texnum][i+j] > best2)
// 					best2 = scrap_allocated[texnum][i+j];
// 			}
// 			if (j == w)
// 			{	// this is a valid spot
// 				*x = i;
// 				*y = best = best2;
// 			}
// 		}
//
// 		if (best + h > BLOCK_HEIGHT)
// 			continue;
//
// 		for (i=0 ; i<w ; i++)
// 			scrap_allocated[texnum][*x + i] = best + h;
//
// 		return texnum;
// 	}
//
// 	Sys_Error ("Scrap_AllocBlock: full");
// }
//
// int	scrap_uploads;
//
// void Scrap_Upload (void)
// {
// 	int		texnum;
//
// 	scrap_uploads++;
//
// 	for (texnum=0 ; texnum<MAX_SCRAPS ; texnum++) {
// 		GL_Bind(scrap_texnum + texnum);
// 		GL_Upload8 (scrap_texels[texnum], BLOCK_WIDTH, BLOCK_HEIGHT, false, true);
// 	}
// 	scrap_dirty = false;
// }

//=============================================================================
/* Support Routines */

typedef struct cachepic_s
{
	char		name[MAX_QPATH];
	qpic_t		pic;
	byte		padding[32];	// for appended glpic
} cachepic_t;

#define	MAX_CACHED_PICS		128
cachepic_t	menu_cachepics[MAX_CACHED_PICS];
int			menu_numcachepics;

byte		menuplyr_pixels[4096];

int		pic_texels;
int		pic_count;

qpic_t *Draw_PicFromWad (char *name)
{
	qpic_t	*p;
	glpic_t	*gl;

	p = W_GetLumpName (name);
	gl = (glpic_t *)p->data;

	// load little ones into the scrap
	// if (p->width < 64 && p->height < 64)
	// {
	// 	int		x, y;
	// 	int		i, j, k;
	// 	int		texnum;
 //
	// 	texnum = Scrap_AllocBlock (p->width, p->height, &x, &y);
	// 	scrap_dirty = true;
	// 	k = 0;
	// 	for (i=0 ; i<p->height ; i++)
	// 		for (j=0 ; j<p->width ; j++, k++)
	// 			scrap_texels[texnum][(y+i)*BLOCK_WIDTH + x + j] = p->data[k];
	// 	texnum += scrap_texnum;
	// 	gl->texnum = texnum;
	// 	gl->sl = (x+0.01)/(float)BLOCK_WIDTH;
	// 	gl->sh = (x+p->width-0.01)/(float)BLOCK_WIDTH;
	// 	gl->tl = (y+0.01)/(float)BLOCK_WIDTH;
	// 	gl->th = (y+p->height-0.01)/(float)BLOCK_WIDTH;
 //
	// 	pic_count++;
	// 	pic_texels += p->width*p->height;
	// }
	// else
	// gl->texnum = GL_LoadPicTexture (p);
	gl->texnum = GL_LoadTexture (name, p->width, p->height, p->data, false, true);
	return p;
}


/*
================
Draw_CachePic
================
*/
qpic_t	*Draw_CachePic (char *path)
{
	cachepic_t	*pic;
	int			i;
	qpic_t		*dat;
	glpic_t		*gl;

	for (pic=menu_cachepics, i=0 ; i<menu_numcachepics ; pic++, i++)
		if (!strcmp (path, pic->name))
			return &pic->pic;

	if (menu_numcachepics == MAX_CACHED_PICS)
		Sys_Error ("menu_numcachepics == MAX_CACHED_PICS");
	menu_numcachepics++;
	strcpy (pic->name, path);

//
// load the pic from disk
//
	dat = (qpic_t *)COM_LoadTempFile (path);	
	if (!dat)
		Sys_Error ("Draw_CachePic: failed to load %s", path);
	SwapPic (dat);

	// HACK HACK HACK --- we need to keep the bytes for
	// the translatable player picture just for the menu
	// configuration dialog
	if (!strcmp (path, "gfx/menuplyr.lmp"))
		memcpy (menuplyr_pixels, dat->data, dat->width*dat->height);

	pic->pic.width = dat->width;
	pic->pic.height = dat->height;

	gl = (glpic_t *)pic->pic.data;
	// gl->texnum = GL_LoadPicTexture (dat);
	gl->texnum = GL_LoadTexture (path, dat->width, dat->height, dat->data, false, true);

	return &pic->pic;
}


void Draw_CharToConback (int num, byte *dest)
{
	int		row, col;
	byte	*source;
	int		drawline;
	int		x;

	row = num>>4;
	col = num&15;
	source = draw_chars + (row<<10) + (col<<3);

	drawline = 8;

	while (drawline--)
	{
		for (x=0 ; x<8 ; x++)
			if (source[x] != 255)
				dest[x] = 0x60 + source[x];
		source += 128;
		dest += 320;
	}

}

typedef struct
{
	char *name;
	int	minimize, maximize;
} glmode_t;

glmode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

/*
===============
Draw_TextureMode_f
===============
*/
void Draw_TextureMode_f (void)
{
	int		i;
	gltexture_t	*glt;

	if (Cmd_Argc() == 1)
	{
		for (i=0 ; i< 6 ; i++)
			if (gl_filter_min == modes[i].minimize)
			{
				Con_Printf ("%s\n", modes[i].name);
				return;
			}
		Con_Printf ("current filter is unknown???\n");
		return;
	}

	for (i=0 ; i< 6 ; i++)
	{
		if (!Q_strcasecmp (modes[i].name, Cmd_Argv(1) ) )
			break;
	}
	if (i == 6)
	{
		Con_Printf ("bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	// for (i=0, glt=gltextures ; i<numgltextures ; i++, glt++)
	// {
	// 	if (glt->mipmap)
	// 	{
	// 		GL_Bind (glt->texnum);
	// 		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
	// 		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	// 	}
	// }
}

/*
===============
Draw_Init
===============
*/
void Draw_Init (void)
{
	int		i;
	qpic_t	*cb;
	byte	*dest, *src;
	int		x, y;
	char	ver[40];
	glpic_t	*gl;
	int		start;
	byte	*ncdata;
	int		f, fstep;


	Cvar_RegisterVariable (&gl_nobind);
	Cvar_RegisterVariable (&gl_max_size);
	Cvar_RegisterVariable (&gl_picmip);

	// 3dfx can only handle 256 wide textures
	// if (!Q_strncasecmp ((char *)gl_renderer, "3dfx",4) ||
	// 	strstr((char *)gl_renderer, "Glide"))
	Cvar_Set ("gl_max_size", "256");

	Cmd_AddCommand ("gl_texturemode", &Draw_TextureMode_f);

	// load the console background and the charset
	// by hand, because we need to write the version
	// string into the background before turning
	// it into a texture
	draw_chars = W_GetLumpName ("conchars");
	// for (i=0 ; i<256*64 ; i++)
	// 	if (draw_chars[i] == 0)
	// 		draw_chars[i] = 255;	// proper transparent color

	// now turn them into textures
	char_texture = GL_LoadTexture ("charset", 128, 128, draw_chars, false, true);

	start = Hunk_LowMark();

	cb = (qpic_t *)COM_LoadTempFile ("gfx/conback.lmp");	
	if (!cb)
		Sys_Error ("Couldn't load gfx/conback.lmp");
	SwapPic (cb);

	// hack the version number directly into the pic
#if defined(__linux__)
	sprintf (ver, "(Linux %2.2f, gl %4.2f) %4.2f", (float)LINUX_VERSION, (float)GLQUAKE_VERSION, (float)VERSION);
#elif defined (PSX)
	sprintf (ver, "(PSXQuake)");
#else
	sprintf (ver, "(gl %4.2f) %4.2f", (float)GLQUAKE_VERSION, (float)VERSION);
#endif
	dest = cb->data + 320*186 + 320 - 11 - 8*strlen(ver);
	y = strlen(ver);
	for (x=0 ; x<y ; x++)
		Draw_CharToConback (ver[x], dest+(x<<3));

#if 1
	conback->width = vid.conwidth;
	conback->height = vid.conheight;

 	// scale console to vid size
 	dest = ncdata = Hunk_AllocName(vid.conwidth * vid.conheight, "conback");
 
 	for (y=0 ; y<vid.conheight ; y++, dest += vid.conwidth)
 	{
 		src = cb->data + cb->width * (y*cb->height/vid.conheight);
 		if (vid.conwidth == cb->width)
 			memcpy (dest, src, vid.conwidth);
 		else
 		{
 			f = 0;
 			fstep = cb->width*0x10000/vid.conwidth;
 			for (x=0 ; x<vid.conwidth ; x+=4)
 			{
 				dest[x] = src[f>>16];
 				f += fstep;
 				dest[x+1] = src[f>>16];
 				f += fstep;
 				dest[x+2] = src[f>>16];
 				f += fstep;
 				dest[x+3] = src[f>>16];
 				f += fstep;
 			}
 		}
 	}
#else
	conback->width = cb->width;
	conback->height = cb->height;
	ncdata = cb->data;
#endif

	// glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	// glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	gl = (glpic_t *)conback->data;
	gl->texnum = GL_LoadTexture ("conback", conback->width, conback->height, ncdata, false, false);
	conback->width = vid.width;
	conback->height = vid.height;

	// free loaded console
	Hunk_FreeToLowMark(start);

	// save a texture slot for translated picture
	// translate_texture = texture_extension_number++;

	// save slots for scraps
	// scrap_texnum = texture_extension_number;
	// texture_extension_number += MAX_SCRAPS;

	//
	// get the other pics we need
	//
	draw_disc = Draw_PicFromWad ("disc");
	draw_backtile = Draw_PicFromWad ("backtile");
}



/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/

void Draw_Character (int x, int y, int num)
{
	struct vram_texture * tex;
	int	row, col;

	if (num == 32)
		return;		// space

	num &= 255;
	
	if (y <= -8)
		return;			// totally off screen

	row = num>>4;
	col = num&15;

	// printf("char tex %c %i %i\n", num, row, col);

	tex = psx_vram_get(char_texture);

	SPRT_8 * sprt = (void*)rb_nextpri;

	setSprt8(sprt);
	setXY0(sprt, x, y);
	setUV0(sprt, col * 8, row * 8);
	setRGB0(sprt, 128, 128, 128);
	sprt->clut = psx_clut_transparent;

	psx_add_prim(sprt, psx_zlevel++);
	rb_nextpri = (void*)++sprt;

	DR_TPAGE * tp = (void*)rb_nextpri;

	setDrawTPage(tp, 0, 1, tex->tpage);

	psx_add_prim(tp, psx_zlevel - 1);
	rb_nextpri = (void*)++tp;
}

/*
================
Draw_String
================
*/
void Draw_String (int x, int y, char *str)
{
	while (*str)
	{
		Draw_Character (x, y, *str);
		str++;
		x += 8;
	}
}

/*
================
Draw_DebugChar

Draws a single character directly to the upper right corner of the screen.
This is for debugging lockups by drawing different chars in different parts
of the code.
================
*/
void Draw_DebugChar (char num)
{
}

void psx_Draw_Pic (int x, int y, qpic_t *pic, qboolean alpha)
{
	glpic_t			* gl = (glpic_t *)pic->data;
	struct vram_texture const * tex = psx_vram_get(gl->texnum);

	// TODO PSX split into multiple textures

	// printf("DrawPic (%04x) %ix%i %ix%i %i;%i %ix%i tp %i\n",
	// 	   tex->ident, x, y, pic->width, pic->height,
	// 	   tex->rect.x, tex->rect.y, tex->rect.w, tex->rect.h, tex->tpage);

	POLY_FT4 * poly = (void*)rb_nextpri;

	setPolyFT4(poly);
	setXYWH(poly, x, y, pic->width, pic->height);
	setUVWH(poly,
		tex->rect.x * 2,
		tex->rect.y,
		tex->rect.w,
		tex->rect.h
	);
	setRGB0(poly, 128, 128, 128);
	poly->clut = tex->alpha ? psx_clut_transparent : psx_clut;
	poly->tpage = tex->tpage;

	psx_add_prim(poly, psx_zlevel++);
	poly++;
	rb_nextpri = (void*)poly;
}

/*
=============
Draw_AlphaPic
=============
*/
void Draw_AlphaPic (int x, int y, qpic_t *pic, float alpha)
{
	// TODO doesn't work like this
	psx_Draw_Pic(x, y, pic, true);
}


/*
=============
Draw_Pic
=============
*/
void Draw_Pic (int x, int y, qpic_t *pic)
{
	psx_Draw_Pic(x, y, pic, false);
}


/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic (int x, int y, qpic_t *pic)
{
	// byte	*dest, *source, tbyte;
	// unsigned short	*pusdest;
	// int				v, u;

	if (x < 0 || (unsigned)(x + pic->width) > vid.width || y < 0 ||
		 (unsigned)(y + pic->height) > vid.height)
	{
		Sys_Error ("Draw_TransPic: bad coordinates %ix%i %ix%i", x, y, pic->width, pic->height);
	}
		
	Draw_Pic (x, y, pic);
}


/*
=============
Draw_TransPicTranslate

Only used for the player color selection menu
=============
*/
void Draw_TransPicTranslate (int x, int y, qpic_t *pic, byte *translation)
{
	int				v, u, c;
	unsigned		trans[64*64], *dest;
	byte			*src;
	int				p;

	printf("Draw_TransPicTranslate\n");
	return;

	GL_Bind (translate_texture);

	c = pic->width * pic->height;

	dest = trans;
	for (v=0 ; v<64 ; v++, dest += 64)
	{
		src = &menuplyr_pixels[ ((v*pic->height)>>6) *pic->width];
		for (u=0 ; u<64 ; u++)
		{
			p = src[(u*pic->width)>>6];
			if (p == 255)
				dest[u] = p;
			else
				dest[u] =  d_8to24table[translation[p]];
		}
	}

	// glTexImage2D (GL_TEXTURE_2D, 0, gl_alpha_format, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);
 //
	// glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
 //
	// glColor3f (1,1,1);
	// glBegin (GL_QUADS);
	// glTexCoord2f (0, 0);
	// glVertex2f (x, y);
	// glTexCoord2f (1, 0);
	// glVertex2f (x+pic->width, y);
	// glTexCoord2f (1, 1);
	// glVertex2f (x+pic->width, y+pic->height);
	// glTexCoord2f (0, 1);
	// glVertex2f (x, y+pic->height);
	// glEnd ();
}


/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground (int lines)
{
	int y = (vid.height * 3) >> 2;

	if (lines > y)
		Draw_Pic(0, lines - vid.height, conback);
	else
		Draw_AlphaPic (0, lines - vid.height, conback, (float)(1.2 * lines)/y);
}


/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h)
{
	printf("Draw_TileClear\n");
	return;

	// glColor3f (1,1,1);
	// GL_Bind (*(int *)draw_backtile->data);
	// glBegin (GL_QUADS);
	// glTexCoord2f (x/64.0, y/64.0);
	// glVertex2f (x, y);
	// glTexCoord2f ( (x+w)/64.0, y/64.0);
	// glVertex2f (x+w, y);
	// glTexCoord2f ( (x+w)/64.0, (y+h)/64.0);
	// glVertex2f (x+w, y+h);
	// glTexCoord2f ( x/64.0, (y+h)/64.0 );
	// glVertex2f (x, y+h);
	// glEnd ();
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, int c)
{
	printf("Draw_Fill\n");
	// glDisable (GL_TEXTURE_2D);
	// glColor3f (host_basepal[c*3]/255.0,
	// 	host_basepal[c*3+1]/255.0,
	// 	host_basepal[c*3+2]/255.0);
 //
	// glBegin (GL_QUADS);
 //
	// glVertex2f (x,y);
	// glVertex2f (x+w, y);
	// glVertex2f (x+w, y+h);
	// glVertex2f (x, y+h);
 //
	// glEnd ();
	// glColor3f (1,1,1);
	// glEnable (GL_TEXTURE_2D);
}
//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (void)
{
	printf("Draw_FadeScreen\n");
	// glEnable (GL_BLEND);
	// glDisable (GL_TEXTURE_2D);
	// glColor4f (0, 0, 0, 0.8);
	// glBegin (GL_QUADS);
 //
	// glVertex2f (0,0);
	// glVertex2f (vid.width, 0);
	// glVertex2f (vid.width, vid.height);
	// glVertex2f (0, vid.height);
 //
	// glEnd ();
	// glColor4f (1,1,1,1);
	// glEnable (GL_TEXTURE_2D);
	// glDisable (GL_BLEND);

	Sbar_Changed();
}

//=============================================================================

/*
================
Draw_BeginDisc

Draws the little blue disc in the corner of the screen.
Call before beginning any disc IO.
================
*/
void Draw_BeginDisc (void)
{
	if (!draw_disc)
		return;
	// glDrawBuffer  (GL_FRONT);
	Draw_Pic (vid.width - 24, 0, draw_disc);
	// glDrawBuffer  (GL_BACK);
}


/*
================
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void Draw_EndDisc (void)
{
}

/*
================
GL_Set2D

Setup as if the screen was 320*200
================
*/
void GL_Set2D (void)
{
	// glViewport (glx, gly, glwidth, glheight);
 //
	// glMatrixMode(GL_PROJECTION);
 //    glLoadIdentity ();
	// glOrtho  (0, vid.width, vid.height, 0, -99999, 99999);
 //
	// glMatrixMode(GL_MODELVIEW);
 //    glLoadIdentity ();
 //
	// glDisable (GL_DEPTH_TEST);
	// glDisable (GL_CULL_FACE);
	// glDisable (GL_BLEND);
	// glEnable (GL_ALPHA_TEST);
 //
	// glColor4f (1,1,1,1);
}

//====================================================================

/*
================
GL_FindTexture
================
*/
int GL_FindTexture (char *identifier)
{
	struct vram_texture * tex = psx_vram_find(identifier, -1, -1);
	if (tex == NULL) {
		return -1;
	}
	return tex->index;
}

/*
================
GL_ResampleTexture
================
*/
void GL_ResampleTexture (unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight)
{
	int		i, j;
	unsigned	*inrow;
	unsigned	frac, fracstep;

	fracstep = inwidth*0x10000/outwidth;
	for (i=0 ; i<outheight ; i++, out += outwidth)
	{
		inrow = in + inwidth*(i*inheight/outheight);
		frac = fracstep >> 1;
		for (j=0 ; j<outwidth ; j+=4)
		{
			out[j] = inrow[frac>>16];
			frac += fracstep;
			out[j+1] = inrow[frac>>16];
			frac += fracstep;
			out[j+2] = inrow[frac>>16];
			frac += fracstep;
			out[j+3] = inrow[frac>>16];
			frac += fracstep;
		}
	}
}

/*
================
GL_Resample8BitTexture -- JACK
================
*/
void GL_Resample8BitTexture (unsigned char *in, int inwidth, int inheight, unsigned char *out,  int outwidth, int outheight)
{
	int		i, j;
	unsigned	char *inrow;
	unsigned	frac, fracstep;

	fracstep = inwidth*0x10000/outwidth;
	for (i=0 ; i<outheight ; i++, out += outwidth)
	{
		inrow = in + inwidth*(i*inheight/outheight);
		frac = fracstep >> 1;
		for (j=0 ; j<outwidth ; j+=4)
		{
			out[j] = inrow[frac>>16];
			frac += fracstep;
			out[j+1] = inrow[frac>>16];
			frac += fracstep;
			out[j+2] = inrow[frac>>16];
			frac += fracstep;
			out[j+3] = inrow[frac>>16];
			frac += fracstep;
		}
	}
}


/*
================
GL_MipMap

Operates in place, quartering the size of the texture
================
*/
// void GL_MipMap (byte *in, int width, int height)
// {
// 	int		i, j;
// 	byte	*out;
//
// 	width <<=2;
// 	height >>= 1;
// 	out = in;
// 	for (i=0 ; i<height ; i++, in+=width)
// 	{
// 		for (j=0 ; j<width ; j+=8, out+=4, in+=8)
// 		{
// 			out[0] = (in[0] + in[4] + in[width+0] + in[width+4])>>2;
// 			out[1] = (in[1] + in[5] + in[width+1] + in[width+5])>>2;
// 			out[2] = (in[2] + in[6] + in[width+2] + in[width+6])>>2;
// 			out[3] = (in[3] + in[7] + in[width+3] + in[width+7])>>2;
// 		}
// 	}
// }

/*
================
GL_MipMap8Bit

Mipping for 8 bit textures
================
*/
void GL_MipMap8Bit (byte *in, int width, int height)
{
	return; // TODO PSX out of memory
	// int		i, j;
	// unsigned short     r,g,b;
	// byte	*out, *at1, *at2, *at3, *at4;
	//	width <<=2;
	// height >>= 1;
	// out = in;
	// for (i=0 ; i<height ; i++, in+=width)
	// {
	// 	for (j=0 ; j<width ; j+=2, out+=1, in+=2)
	// 	{
	// 		at1 = (byte *) (d_8to24table + in[0]);
	// 		at2 = (byte *) (d_8to24table + in[1]);
	// 		at3 = (byte *) (d_8to24table + in[width+0]);
	// 		at4 = (byte *) (d_8to24table + in[width+1]);
 //
 // 			r = (at1[0]+at2[0]+at3[0]+at4[0]); r>>=5;
 // 			g = (at1[1]+at2[1]+at3[1]+at4[1]); g>>=5;
 // 			b = (at1[2]+at2[2]+at3[2]+at4[2]); b>>=5;
 //
	// 		out[0] = d_15to8table[(r<<0) + (g<<5) + (b<<10)];
	// 	}
	// }
}

void GL_Upload8_EXT (byte *data, int width, int height,  qboolean mipmap, qboolean alpha) 
{
	return;
	int			i, s;
	qboolean	noalpha;
	int			p;
	static unsigned j;
	int			samples;
	uint8_t scaled[2]; // TODO PSX out of memory
    // static	unsigned char scaled[1024*512];	// [512*256];
	int			scaled_width, scaled_height;

	s = width*height;
	// if there are no transparent pixels, make it a 3 component
	// texture even if it was specified as otherwise
	if (alpha)
	{
		noalpha = true;
		for (i=0 ; i<s ; i++)
		{
			if (data[i] == 255)
				noalpha = false;
		}

		if (alpha && noalpha)
			alpha = false;
	}
	for (scaled_width = 1 ; scaled_width < width ; scaled_width<<=1)
		;
	for (scaled_height = 1 ; scaled_height < height ; scaled_height<<=1)
		;

	scaled_width >>= (int)gl_picmip.value;
	scaled_height >>= (int)gl_picmip.value;

	if (scaled_width > gl_max_size.value)
		scaled_width = gl_max_size.value;
	if (scaled_height > gl_max_size.value)
		scaled_height = gl_max_size.value;

	if (scaled_width * scaled_height > sizeof(scaled))
		Sys_Error ("GL_LoadTexture: too big");

	samples = 1; // alpha ? gl_alpha_format : gl_solid_format;

	texels += scaled_width * scaled_height;

	if (scaled_width == width && scaled_height == height)
	{
		if (!mipmap)
		{
			glTexImage2D (GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, scaled_width, scaled_height, 0, GL_COLOR_INDEX , GL_UNSIGNED_BYTE, data);
			goto done;
		}
		memcpy (scaled, data, width*height);
	}
	else
		GL_Resample8BitTexture (data, width, height, scaled, scaled_width, scaled_height);

	glTexImage2D (GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, scaled_width, scaled_height, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, scaled);
	if (mipmap)
	{
		int		miplevel;

		miplevel = 0;
		while (scaled_width > 1 || scaled_height > 1)
		{
			GL_MipMap8Bit ((byte *)scaled, scaled_width, scaled_height);
			scaled_width >>= 1;
			scaled_height >>= 1;
			if (scaled_width < 1)
				scaled_width = 1;
			if (scaled_height < 1)
				scaled_height = 1;
			miplevel++;
			glTexImage2D (GL_TEXTURE_2D, miplevel, GL_COLOR_INDEX8_EXT, scaled_width, scaled_height, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, scaled);
		}
	}
done: ;


	if (mipmap)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
}

/*
===============
GL_Upload8
===============
*/
qboolean VID_Is8bit(void);

void GL_Upload8 (byte *data, int width, int height,  qboolean mipmap, qboolean alpha)
{
	return;
// static	unsigned	trans[640*480];		// FIXME, temporary
	// unsigned trans[8]; // TODO PSX out of memory
	int			i, s;
	qboolean	noalpha;
	int			p;

	s = width*height;
	// if there are no transparent pixels, make it a 3 component
	// texture even if it was specified as otherwise
	// if (alpha)
	// {
	// 	noalpha = true;
	// 	for (i=0 ; i<s ; i++)
	// 	{
	// 		p = data[i];
	// 		if (p == 255)
	// 			noalpha = false;
	// 		trans[i] = d_8to24table[p];
	// 	}
 //
	// 	if (alpha && noalpha)
	// 		alpha = false;
	// }
	// else
	// {
	// 	if (s&3)
	// 		Sys_Error ("GL_Upload8: s&3");
	// 	for (i=0 ; i<s ; i+=4)
	// 	{
	// 		trans[i] = d_8to24table[data[i]];
	// 		trans[i+1] = d_8to24table[data[i+1]];
	// 		trans[i+2] = d_8to24table[data[i+2]];
	// 		trans[i+3] = d_8to24table[data[i+3]];
	// 	}
	// }

 	// if (VID_Is8bit() && !alpha && (data!=scrap_texels[0])) {
 	if (VID_Is8bit() && !alpha) {
 		GL_Upload8_EXT (data, width, height, mipmap, alpha);
 		return;
	}
	// GL_Upload32 (data, width, height, mipmap, alpha);
}

/*
================
GL_LoadTexture
================
*/
int GL_LoadTexture (char *identifier, int width, int height, byte *data, qboolean mipmap, qboolean alpha)
{
	static uint8_t scaled[2 * VRAM_PAGE_WIDTH * VRAM_PAGE_HEIGHT];
	int div = 1;
	struct vram_texture * tex;

	// see if the texture is already present
	tex = psx_vram_find(identifier, width, height);
	if (tex) {
		printf("GL_LoadTexture \"%s\" already loaded, returning cached\n",
			   identifier, width, height);
		return tex->index;
	}

	if (width > 2 * VRAM_PAGE_WIDTH || height > VRAM_PAGE_HEIGHT) {
		int divw = (width + (VRAM_PAGE_WIDTH - 1)) / VRAM_PAGE_WIDTH;
		int divh = (width + (VRAM_PAGE_HEIGHT - 1)) / VRAM_PAGE_HEIGHT;
		div = divw > divh ? divw : divh;
		int neww = width / div;
		int newh = height / div;

		printf("Texture %s larger than texpage, scaling %ix%i down to %ix%i\n",
			   identifier, width, height, neww, newh);

		for (int y = 0; y < newh; ++y) {
			for (int x = 0; x < neww; ++x) {
				uint8_t val = data[(y * width * div) + (x * div)];
				if (alpha && val == 0xFF) {
					val = 0;
				}
				scaled[y * neww + x] = val;
			}
		}

		width = neww;
		height = newh;
		data = scaled;
	} else if (alpha) {
		for (int i = 0; i < width * height; ++i) {
			if (data[i] == 0xFF) {
				data[i] = 0;
			}
		}
	}

	tex = psx_vram_pack (identifier, width, height);
	if (tex == NULL) {
		printf("Failed to pack image %ix%i, ident %s\n", width, height, identifier);
		return -1;
	}

	RECT load_rect = tex->rect;
	load_rect.x += tex->page->rect.x;
	load_rect.y += tex->page->rect.y;
	load_rect.w /= 2;

	LoadImage(&load_rect, (void*)data);
	tex->div = div;
	tex->alpha = alpha;

	printf("GL_LoadTexture \"%s\" (%04x) %ix%i\n", identifier, tex->ident, width, height);

	if (tex->rect.x + tex->rect.w >= UINT8_MAX) {
		tex->rect.w = UINT8_MAX - tex->rect.x;
	}
	if (tex->rect.y + tex->rect.h >= UINT8_MAX) {
		tex->rect.h = UINT8_MAX - tex->rect.y;
	}

	return tex->index;
}

/*
================
GL_LoadPicTexture
================
*/
// int GL_LoadPicTexture (qpic_t *pic)
// {
// 	return GL_LoadTexture ("", pic->width, pic->height, pic->data, false, true);
// }

/****************************************/

static GLenum oldtarget = TEXTURE0_SGIS;

void GL_SelectTexture (GLenum target) 
{
	if (!gl_mtexable)
		return;
	qglSelectTextureSGIS(target);
	if (target == oldtarget) 
		return;
	cnttextures[oldtarget-TEXTURE0_SGIS] = currenttexture;
	currenttexture = cnttextures[target-TEXTURE0_SGIS];
	oldtarget = target;
}
