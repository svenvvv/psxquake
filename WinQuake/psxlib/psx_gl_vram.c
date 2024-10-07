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

	ident_hash = crc32((uint8_t*)ident, strlen(ident));

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
		available_rect->x + page->x,
		available_rect->y + page->y,
		available_rect->w,
		available_rect->h
	);
}


struct vram_texture * allocate_and_split(int id, int w, int h,
										 struct vram_texpage * page, RECT * rect)
{
	struct vram_texture * tex = vram_tex_alloc();
	int remaining_w = rect->w - w;
	int remaining_h = rect->h - h;

	printf("VRAM: packing id=%d to %d;%d (%dx%d), abs vram %d;%d\n", id,
			rect->x, rect->y, rect->w, rect->h, page->x + rect->x, page->y + rect->y);

	tex->ident = id;
	tex->rect.x = rect->x;
	tex->rect.y = rect->y;
	tex->rect.w = w * 2;
	tex->rect.h = h;
	tex->page = page;

	// The framebuffer coordinates should be a multiple of 64 for the X axis and a multiple
	// of 256 for the Y axis, the coordinates will be rounded down to the nearest lower
	// multiple otherwise.
	int tpx = (page->x + tex->rect.x) / 128;
	tex->tpage = getTPage(1, 0,
						  (tpx) * 128,
						  page->y);
	printf("tpx %d %d %d\n", tpx, page->y, tex->tpage);

	// printf("Packed (%u) to %i;%i;%i;%i\n",
	// 		id,
	// 		page->x + rect->x,
	// 		page->y + rect->y,
	// 		rect->w, rect->h);

	EnterCriticalSection();
	page->textures[page->textures_count++] = tex;
	ExitCriticalSection();
	if (page->textures_count >= ARRAY_SIZE(page->textures)) {
		Sys_Error("No more textures in page, %d\n", page->textures_count);
	}

	if (remaining_w == 0 && remaining_h == 0) {
		printf("Discarding %i;%i;%i;%i, remaining %ix%i\n",
				rect->x, rect->y,
				rect->w, rect->h,
				remaining_w, remaining_h);
		// Discard if small
		// memmove(page->rects + r, page->rects + r + 1,
		// 		sizeof(*rect) * (page->rects_count - r));
		// page->rects_count -= 1;
		// psx_vram_rect(
		// 	page->x + rect->x,
		// 	page->y + rect->y,
		// 	rect->w,
		// 	rect->h
		// );
		memmove(rect, rect + 1,
				(sizeof(*rect) * page->available_rects_count) - (rect - page->available_rects));
		page->available_rects_count -= 1;
	} else {
		// Split available rect around the newly allocated texture
		printf("Splitting %i;%i;%i;%i...\n",
				rect->x, rect->y,
				rect->w, rect->h);

		// Right side left-overs
		if (remaining_w > 0) {
			rect->x += w;
			rect->w -= w;
			rect->h = h;
			printf("...right %i;%i;%i;%i\n",
				rect->x, rect->y,
				rect->w, rect->h);
			// psx_vram_rect(
			// 	page->x + rect->x,
			// 	page->y + rect->y,
			// 	rect->w,
			// 	rect->h
			// );
		}

		// Bottom left-overs
		if (remaining_h > 0) {
			if (remaining_w > 0) {
				// Allocate new rect for bottom
				EnterCriticalSection();
				rect = &page->available_rects[page->available_rects_count++];
				ExitCriticalSection();
				if (page->available_rects_count >= ARRAY_SIZE(page->available_rects)) {
					Sys_Error("No more rects in page, %d\n", page->available_rects_count);
				}
			}
			rect->x = tex->rect.x;
			rect->y = tex->rect.y + h;
			rect->w = remaining_w + w;
			rect->h = remaining_h;
			printf("...bottom %i;%i;%i;%i\n",
				rect->x, rect->y,
				rect->w, rect->h);
			// psx_vram_rect(
			// 	page->x + rect->x,
			// 	page->y + rect->y,
			// 	rect->w,
			// 	rect->h
			// );
		}
	}

	return tex;
}

struct vram_texture * psx_vram_pack (char * ident, int w, int h)
{
	uint32_t ident_hash = 0;
	struct vram_texture * target_tex = NULL;
	struct vram_texpage * candidate_page = NULL;
	RECT * candidate_rect = NULL;
	int candidate_remaining_h = 0;

	if (ident[0]) {
		ident_hash = crc32((uint8_t*)ident, strlen(ident));
	}

	w /= 2;

	for (int p = 0; p < ARRAY_SIZE(vram_pages); ++p) {
 		struct vram_texpage * page = &vram_pages[p];

		// printf("Page %d avail %d\n", p, page->available_rects_count);

		if (page->available_rects_count == 0) {
			continue;
		}

		for (int r = 0; r < page->available_rects_count; ++r) {
			RECT * available_rect = &page->available_rects[r];
			int remaining_w = available_rect->w - w;
			int remaining_h = available_rect->h - h;

			// printf("VRAM: packing \"%s\" (%dx%d), %dx%d remaining %i;%i\n", ident,
			// 	   w, h,
			// 	   available_rect->w, available_rect->h,
			// 	   remaining_w, remaining_h);

			if (remaining_w < 0 || remaining_h < 0) {
				continue;
			}

			// If our rect takes up more Y-space then candidate is better
			if (candidate_rect && (remaining_h == 0 || candidate_remaining_h <= remaining_h)) {
				continue;
			}

			candidate_page = page;
			candidate_rect = available_rect;
			candidate_remaining_h = remaining_h;

			printf("VRAM: selected page %d rect %d;%d (%dx%d)\n", p,
				   candidate_rect->x, candidate_rect->y,
				   candidate_rect->w, candidate_rect->h);
		}
	}

	if (candidate_page) {
		target_tex = allocate_and_split(ident_hash, w, h, candidate_page, candidate_rect);
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
	for (int y = 0; y < 2; ++y) {
		for (int x = 0; x < 6; ++x) {
			struct vram_texpage * page = &vram_pages[(y * 6) + x];
			page->x = (x * VRAM_PAGE_WIDTH) + 256;
			page->y = y * VRAM_PAGE_HEIGHT;
			page->textures_count = 0;

			page->available_rects[0] = {
				0, 0,
				VRAM_PAGE_WIDTH, VRAM_PAGE_HEIGHT
			};
			page->available_rects_count = 1;
		}
	}

	// Partially reserved pages (rightmost of fb)
	int reserved_x = VID_WIDTH % (VRAM_PAGE_WIDTH - 1);
	vram_pages[0].available_rects[0] = {
		int16_t(reserved_x),
		0,
		int16_t(VRAM_PAGE_WIDTH - reserved_x),
		VRAM_PAGE_HEIGHT,
	};
	vram_pages[6].available_rects[0] = vram_pages[0].available_rects[0];

	for (int i = 0; i < ARRAY_SIZE(vram_pages); ++i) {
		struct vram_texpage const * page = &vram_pages[i];
		RECT const * r = &page->available_rects[0];
		psx_vram_rect(
			page->x + r->x, page->y + r->y,
			r->w, r->h
		);
	}
}
