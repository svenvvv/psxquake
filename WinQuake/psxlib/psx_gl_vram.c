#include "psx_gl.h"
#include "psx_io.h"
#include <stdio.h>
#include <string.h>
#include "../sys.h"

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
				// TODO PSX tex->rect.h is sometimes one less than h ???
 				printf("psx_vram_find: cache mismatch 0x%X: %ix%i != %ix%i\n",
					   tex->ident, w, h, tex->rect.w, tex->rect.h);
			}
			return tex;
		}
	}

	return NULL;
}

void psx_vram_rect(int x, int y, int w, int h)
{
	FILL * fill = (FILL*)rb_nextpri;
	setFill(fill);
	setXY0(fill, x, y);
	setWH(fill, w, h);
	setRGB0(fill, rand() % 0xFF, rand() % 0xFF, rand() % 0xFF);
	psx_add_prim(fill, 0);
	rb_nextpri = (void*)++fill;

	psx_rb_present();
}

void psx_vram_compact(struct vram_texpage * page)
{
	int largest_w = 0;
	RECT * available_rect;

	for (int t = 0; t < page->textures_count; ++t) {
		struct vram_texture * tex = page->textures[t];
		int w = tex->rect.x + tex->rect.w;
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

			// printf("packing \"%s\", remaining %i;%i\n", ident, remaining_w, remaining_h);

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
										(page->rect.x + target_tex->rect.x) / 128 * 128,
										page->rect.y + target_tex->rect.y);

			printf("Packed %s (%u) to %i;%i;%i;%i\n",
				   ident, ident_hash,
					page->rect.x + available_rect->x, page->rect.y + available_rect->y,
					available_rect->w, available_rect->h);

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

					// psx_vram_rect(
					// 	available_rect->x + page->rect.x,
					// 	available_rect->y + page->rect.y,
					// 	available_rect->w,
					// 	available_rect->h
					// );
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
					// psx_vram_rect(
					// 	available_rect->x + page->rect.x,
					// 	available_rect->y + page->rect.y,
					// 	available_rect->w,
					// 	available_rect->h
					// );
				}
			}

			goto exit;
		}
	}

	// if (target_tex == NULL) {
	// 	for (int p = 0; p < ARRAY_SIZE(vram_pages); ++p) {
	// 		printf("Unable to fit new texture, recompacting page %d...\n", p);
	// 		struct vram_texpage * page = &vram_pages[p];
	// 		psx_vram_compact(page);
	// 	}
	// }

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

		if (page->rect.x < VID_WIDTH) {
			struct vram_texture * tex = vram_tex_alloc();
			page->textures[0] = tex;
			page->textures_count += 1;

			tex->rect.x = 0;
			tex->rect.y = 0;
			tex->rect.h = VRAM_PAGE_HEIGHT;
			tex->rect.w = VRAM_PAGE_WIDTH; // VID_WIDTH - page->rect.x;
			// tex->rect.w = VID_WIDTH - page->rect.x;
			// if (tex->rect.w > VRAM_PAGE_WIDTH) {
			// 	tex->rect.w = VRAM_PAGE_WIDTH;
			// 	page->available_rects_count = 0;
			// } else {
			// 	page->available_rects[0].x = tex->rect.w;
			// 	page->available_rects[0].y = 0;
			// 	page->available_rects[0].w = VRAM_PAGE_WIDTH - tex->rect.w;
			// 	page->available_rects[0].h = VRAM_PAGE_HEIGHT;
			// 	page->available_rects_count = 1;
			// }
   //
			// printf("TexPage fb init %i;%i %ix%i: used %i;%i %ix%i, avail %i;%i %ix%i\n",
			// 	page->rect.x, page->rect.y, page->rect.w, page->rect.h,
			// 	tex->rect.x, tex->rect.y, tex->rect.w, tex->rect.h,
			// 	page->available_rects[0].x, page->available_rects[0].y, page->available_rects[0].w, page->available_rects[0].h);
		} else {
			page->available_rects[0].x = 0;
			page->available_rects[0].y = 0;
			page->available_rects[0].w = VRAM_PAGE_WIDTH;
			page->available_rects[0].h = VRAM_PAGE_HEIGHT;
			page->available_rects_count = 1;

			printf("TexPage empty init %i;%i %ix%i\n",
				page->rect.x, page->rect.y, page->rect.w, page->rect.h);
		}

		psx_vram_rect(
			page->rect.x,
			page->rect.y,
			page->rect.w,
			page->rect.h
		);
	}

	psx_rb_present();
}
