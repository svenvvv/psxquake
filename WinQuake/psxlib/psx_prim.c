#include "psx_gl.h"
#include "../quakedef.h"

#include <inline_c.h>

inline SVECTOR midpoint(SVECTOR const * a, SVECTOR const * b)
{
	return {
		int16_t((int(a->vx) + b->vx) / 2),
		int16_t((int(a->vy) + b->vy) / 2),
		int16_t((int(a->vz) + b->vz) / 2),
	};
}

static void draw_tri_tex_subdiv_2(SVECTOR const verts[3], SVECTOR const & normal,
                                  uint8_t const uv[3 * 2], struct vram_texture const * tex,
                                  int subdiv, uint8_t * scratch)
{
	if (subdiv <= 1) {
		draw_tri_tex(verts, normal, uv, tex);
		return;
	}

	SVECTOR * midpoints = (SVECTOR*) scratch;
	scratch += sizeof(SVECTOR) * 3;
	SVECTOR * dverts = (SVECTOR*) scratch;
	scratch += sizeof(SVECTOR) * 3;

	midpoints[0] = midpoint(&verts[0], &verts[1]);
	midpoints[1] = midpoint(&verts[0], &verts[2]);
	midpoints[2] = midpoint(&verts[1], &verts[2]);

	dverts[0] = verts[0];
	dverts[1] = midpoints[0];
	dverts[2] = midpoints[1];
	draw_tri_tex_subdiv_2(dverts, normal, uv, tex, subdiv - 1, scratch);

	dverts[0] = midpoints[2];
	dverts[1] = midpoints[1];
	dverts[2] = midpoints[0];
	draw_tri_tex_subdiv_2(dverts, normal, uv, tex, subdiv - 1, scratch);

	dverts[0] = midpoints[1];
	dverts[1] = midpoints[2];
	dverts[2] = verts[2];
	draw_tri_tex_subdiv_2(dverts, normal, uv, tex, subdiv - 1, scratch);

	dverts[0] = midpoints[0];
	dverts[1] = verts[1];
	dverts[2] = midpoints[2];
	draw_tri_tex_subdiv_2(dverts, normal, uv, tex, subdiv - 1, scratch);
}

void draw_tri_tex_subdiv(SVECTOR const verts[3], SVECTOR const & normal,
						 uint8_t const uv[3 * 2], struct vram_texture const * tex,
						 int subdiv)
{
	if (subdiv <= 0) {
		Sys_Error("Subdiv <0, %d", subdiv);
	}
	draw_tri_tex_subdiv_2(verts, normal, uv, tex, subdiv, psx_scratch);
}

static void draw_quad_tex_subdiv_2(SVECTOR const verts[4], SVECTOR const & normal,
                                   uint8_t const uv[4 * 2], struct vram_texture const * tex,
                                   int subdiv, uint8_t * scratch)
{
	if (subdiv <= 1) {
		draw_quad_tex(verts, normal, uv, tex);
		return;
	}

	SVECTOR * midpoints = (SVECTOR*) scratch;
	scratch += sizeof(SVECTOR) * 5;

	SVECTOR * dverts = (SVECTOR*) scratch;
	scratch += sizeof(SVECTOR) * 4;

	midpoints[0] = midpoint(&verts[0], &verts[1]);
	midpoints[1] = midpoint(&verts[0], &verts[2]);
	midpoints[2] = midpoint(&verts[1], &verts[3]);
	midpoints[3] = midpoint(&verts[2], &verts[3]);
	midpoints[4] = midpoint(&verts[0], &verts[3]);

	dverts[0] = verts[0];
	dverts[1] = midpoints[0];
	dverts[2] = midpoints[1];
	dverts[3] = midpoints[4];
	draw_quad_tex_subdiv_2(dverts, normal, uv, tex, subdiv - 1, scratch);

	dverts[0] = midpoints[1];
	dverts[1] = midpoints[4];
	dverts[2] = verts[2];
	dverts[3] = midpoints[3];
	draw_quad_tex_subdiv_2(dverts, normal, uv, tex, subdiv - 1, scratch);

	dverts[0] = midpoints[3];
	dverts[1] = midpoints[4];
	dverts[2] = verts[3];
	dverts[3] = midpoints[2];
	draw_quad_tex_subdiv_2(dverts, normal, uv, tex, subdiv - 1, scratch);

	dverts[0] = midpoints[4];
	dverts[1] = midpoints[0];
	dverts[2] = midpoints[2];
	dverts[3] = verts[1];
	draw_quad_tex_subdiv_2(dverts, normal, uv, tex, subdiv - 1, scratch);
}

void draw_quad_tex_subdiv(SVECTOR const verts[4], SVECTOR const & normal,
						  uint8_t const uv[4 * 2], struct vram_texture const * tex,
						  int subdiv)
{
	if (subdiv <= 0) {
		Sys_Error("Subdiv <0, %d", subdiv);
	}
	draw_quad_tex_subdiv_2(verts, normal, uv, tex, subdiv, psx_scratch);
}

int draw_quad_tex(SVECTOR const verts[4], SVECTOR const & normal,
				  uint8_t const uv[4 * 2], struct vram_texture const * tex)
{
	int gv;
	POLY_FT4 * poly = (POLY_FT4 *) rb_nextpri;

	gte_ldv3(&verts[0], &verts[1], &verts[2]);

	gte_rtpt();
	gte_nclip();
	gte_stopz(&gv);
	if (gv > 0) {
		return -1;
	}

	setPolyFT4(poly);

	gte_stsxy3(&poly->x0, &poly->x1, &poly->x2);

	gte_ldv0(&verts[3]);
	gte_rtps();

	gte_avsz4();
	gte_stotz(&gv);
	if ((gv >> 2) > OT_LEN) {
		return -1;
	}

	gte_stsxy(&poly->x3);

	gte_ldrgb(&poly->r0);
	gte_ldv0(&normal);
	gte_ncs();
	gte_strgb(&poly->r0);

	// uint8_t const ambient[] = { 255, 242, 230 };
	// setRGB0(poly, 255, 242, 230);

	setUVWH(poly,
		tex->rect.x * 2,
		tex->rect.y,
		tex->rect.w,
		tex->rect.h
	);
	// int ux = 0; // tex->rect.x * 2;
	// int uy = 0; // tex->rect.y;
	// setUV4(poly,
	// 	ux + uv[0], uy + uv[1],
	// 	ux + uv[2], uy + uv[3],
	// 	ux + uv[4], uy + uv[5],
	// 	ux + uv[6], uy + uv[7]
	// );
	// printf("uv\n");
	// printf(" %6d %6d\n", uv[0], uv[1]);
	// printf(" %6d %6d\n", uv[2], uv[3]);
	// printf(" %6d %6d\n", uv[4], uv[5]);
	// printf(" %6d %6d\n", uv[6], uv[7]);
	poly->tpage = tex->tpage;
	poly->clut = psx_clut;

	// setRGB0(poly, 127, 127, 127);

	// printf("tri %d %d %d => %d %d %d\n",
	// 	  a->v[0], verts->v[1], verts->v[2],
	// 	   poly->x0, poly->x1, poly->x2);

	psx_add_prim_z(poly, gv >> 2);

	return gv >> 2;
}

void draw_quad(SVECTOR const verts[4], CVECTOR const * color)
{
	int gv;
	// POLY_F4 * poly = (POLY_F4 *) rb_nextpri;
	LINE_F4 * poly = (LINE_F4 *) rb_nextpri;

	gte_ldv3(&verts[0], &verts[1], &verts[2]);

	gte_rtpt();
	gte_nclip();
	gte_stopz(&gv);
	if (gv < 0) {
		return;
	}

	// setPolyF4(poly);
	setLineF4(poly);

	gte_stsxy3(&poly->x0, &poly->x1, &poly->x2);

	gte_ldv0(&verts[3]);
	gte_rtps();

	gte_avsz4();
	gte_stotz(&gv);
	if ((gv >> 2) > OT_LEN) {
		return;
	}

	gte_stsxy(&poly->x3);

	setRGB0(poly, color->r, color->g, color->b);

	// SVECTOR norm = 	{ 0, -ONE, 0, 0 };

	// gte_ldrgb(&(poly->r0));
	// gte_ldv0(&norm);
	// gte_ncs();
	// gte_strgb(&(poly->r0));

	// printf("tri %d %d %d => %d %d %d\n",
	// 	  a->v[0], verts->v[1], verts->v[2],
	// 	   poly->x0, poly->x1, poly->x2);

	psx_add_prim_z(poly, gv >> 2);
}

void draw_tri_tex(SVECTOR const verts[3], SVECTOR const & normal,
				  uint8_t const uv[3 * 2], struct vram_texture const * tex)
{
	int gv;
	auto * poly = psx_nextpri<POLY_FT3>();

	gte_ldv3(&verts[0], &verts[1], &verts[2]);

	gte_rtpt();
	gte_nclip();
	gte_stopz(&gv);
	if (gv < 0) {
		return;
	}

	gte_avsz3();
	gte_stotz(&gv);
	if ((gv >> 2) > OT_LEN) {
		return;
	}

	setPolyFT3(poly);

	gte_stsxy3(&poly->x0, &poly->x1, &poly->x2);

	// unsigned uv_tx = tex->rect.x * 2;
	// unsigned uv_ty = tex->rect.y;
	// setUV3(poly,
	// 	uv_tx + uv[0], uv_ty + uv[1],
	// 	uv_tx + uv[2], uv_ty + uv[3],
	// 	uv_tx + uv[4], uv_ty + uv[5]
	// );
	setUV3(poly,
		tex->rect.x * 2, tex->rect.y,
		tex->rect.x * 2 + tex->rect.w, tex->rect.y,
		tex->rect.x * 2, tex->rect.y + tex->rect.h
	);
	poly->tpage = tex->tpage;
	poly->clut = psx_clut;

	// setRGB0(poly, 127, 127, 127);

	gte_ldrgb(&(poly->r0));
	gte_ldv0(&normal);
	gte_ncs();
	gte_strgb(&(poly->r0));

	// printf("tri %d %d %d => %d %d %d\n",
	// 	  a->v[0], verts->v[1], verts->v[2],
	// 	   poly->x0, poly->x1, poly->x2);

	psx_add_prim_z(poly, gv >> 2);
}

void draw_tri(SVECTOR const verts[3], CVECTOR const * color)
{
	int gv;
	auto * poly = psx_nextpri<LINE_F3>();

	gte_ldv3(&verts[0], &verts[1], &verts[2]);

	gte_rtpt();
	gte_nclip();
	gte_stopz(&gv);
	if (gv < 0) {
		return;
	}

	gte_avsz3();
	gte_stotz(&gv);
	if ((gv >> 2) > OT_LEN) {
		return;
	}

	setLineF3(poly);

	gte_stsxy3(&poly->x0, &poly->x1, &poly->x2);

	setRGB0(poly, color->r, color->g, color->b);

	// SVECTOR norm = 	{ 0, -ONE, 0, 0 };

	// gte_ldrgb(&(poly->r0));
	// gte_ldv0(&norm);
	// gte_ncs();
	// gte_strgb(&(poly->r0));

	// printf("tri %d %d %d => %d %d %d\n",
	// 	  a->v[0], verts->v[1], verts->v[2],
	// 	   poly->x0, poly->x1, poly->x2);

	psx_add_prim_z(poly, gv >> 2);
}
