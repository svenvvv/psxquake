#include "psx_gl.h"
#include "psx_io.h"
#include <stdio.h>

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
