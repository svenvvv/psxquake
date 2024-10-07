#include "psx_gl.h"
#include "psx_io.h"
#include "../sys.h"
#include <stdio.h>

int psx_db = 0;
struct PQRenderBuf rb[2];
uint8_t * rb_nextpri;
uint16_t psx_zlevel = 0;

int pricount = 0;

static void psx_rb_draw_init(DRAWENV * draw)
{
#ifdef GLQUAKE
    setRGB0(draw, 63, 0, 127);
#else
    // Set bg to black so we don't have to loop through each pixel of each frame replacing black
    // pixels (transparent) with dark red.
    setRGB0(draw, 0, 0, 0);
#endif
    draw->isbg = 1;
    draw->dtd = 1;
}

void psx_rb_init(void)
{
    ResetGraph(0);

    SetDefDispEnv(&rb[0].disp, 0, 0, VID_WIDTH, VID_HEIGHT);
    SetDefDrawEnv(&rb[0].draw, 0, VID_HEIGHT, VID_WIDTH, VID_HEIGHT);
    psx_rb_draw_init(&rb[0].draw);

    SetDefDispEnv(&rb[1].disp, 0, VID_HEIGHT, VID_WIDTH, VID_HEIGHT);
    SetDefDrawEnv(&rb[1].draw, 0, 0, VID_WIDTH, VID_HEIGHT);
    psx_rb_draw_init(&rb[1].draw);

    PutDispEnv(&rb[0].disp);
    PutDrawEnv(&rb[0].draw);

    ClearOTagR(rb[0].ot, ARRAY_SIZE(rb->ot));
    ClearOTagR(rb[1].ot, ARRAY_SIZE(rb->ot));

    rb_nextpri = rb[0].pribuf;
}

void psx_rb_present(void)
{
    // printf("Frame pricount %d %d\n", pricount, (rb[psx_db].pribuf + sizeof(rb->pribuf)) - rb_nextpri);
    pricount = 0;

    // if (pricount > 400) {
    //     return;
    // }

    if (rb_nextpri >= rb[psx_db].pribuf + sizeof(rb->pribuf)) {
        Sys_Error("Prim buf overflow, %d\n", rb_nextpri - (rb[psx_db].pribuf + sizeof(rb->pribuf)));
    }

    if (psx_zlevel >= OT_LEN) {
        Sys_Error("psx_zlevel out of bounds\n");
    }

    DrawSync(0);
    VSync(0);

    psx_db = !psx_db;
    rb_nextpri = rb[psx_db].pribuf;
    psx_zlevel = 0;

    ClearOTagR(rb[psx_db].ot, OT_LEN);

    PutDrawEnv(&rb[psx_db].draw);
    PutDispEnv(&rb[psx_db].disp);

    SetDispMask(1);

    DrawOTag(rb[1-psx_db].ot + (OT_LEN - 1));
}
