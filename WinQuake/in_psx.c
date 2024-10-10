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
// in_null.c -- for systems without a mouse

#include "quakedef.h"
#include <stdbool.h>

#include <psxpad.h>

static uint8_t padbuf[2][64];

void IN_Init(void)
{
    printf("Initializing PSX PAD...");

    EnterCriticalSection();

    InitPAD(padbuf[0], sizeof(padbuf[0]), padbuf[1], sizeof(padbuf[1]));
    StartPAD();
    ChangeClearPAD(0);

    ExitCriticalSection();

    printf("OK\n");
}

void IN_Shutdown(void)
{
}

#include <psxgte.h>
#include <inline_c.h>

int psx_in_r = 0;
int psx_in_l = 0;

#define EMIT_IF_CHANGED(psxkey, qkey) \
    if (changed & psxkey) { \
        Key_Event (qkey, !(pad->btn & psxkey)); \
    }

void IN_Commands(void)
{
    static uint16_t prev_btn = UINT16_MAX;
    PADTYPE * pad = (PADTYPE *)padbuf[0];

    if (pad->stat == 0) {
        if ((pad->type == PAD_ID_DIGITAL) || (pad->type == PAD_ID_ANALOG_STICK) || (pad->type == PAD_ID_ANALOG)) {
            if (pad->btn == prev_btn) {
                return;
            }
            uint16_t changed = ~(prev_btn & pad->btn);

            if (!(pad->btn & PAD_R1)) {
                psx_in_r += 8;
                gte_SetGeomScreen(psx_in_r);
            }
            if (!(pad->btn & PAD_R2)) {
                psx_in_r -= 8;
                gte_SetGeomScreen(psx_in_r);
            }
            if (!(pad->btn & PAD_L1)) {
                psx_in_l += 256;
            }
            if (!(pad->btn & PAD_L2)) {
                psx_in_l -= 256;
            }

            EMIT_IF_CHANGED(PAD_START, K_ESCAPE);

            EMIT_IF_CHANGED(PAD_CROSS, K_ENTER);
            EMIT_IF_CHANGED(PAD_CIRCLE, K_MOUSE1);
            EMIT_IF_CHANGED(PAD_SQUARE, K_MOUSE2);
            EMIT_IF_CHANGED(PAD_TRIANGLE, K_MOUSE3);

            EMIT_IF_CHANGED(PAD_UP, K_UPARROW);
            EMIT_IF_CHANGED(PAD_DOWN, K_DOWNARROW);
            EMIT_IF_CHANGED(PAD_LEFT, K_LEFTARROW);
            EMIT_IF_CHANGED(PAD_RIGHT, K_RIGHTARROW);

            prev_btn = pad->btn;
        }
    }
}

void IN_Move(usercmd_t * cmd)
{
    return;
	float	speed, aspeed;

	if (in_speed.state & 1)
		speed = cl_movespeedkey.value;
	else
		speed = 1;
	aspeed = speed*host_frametime;

    PADTYPE * pad = (PADTYPE *)padbuf[0];

    if (pad->stat == 0) {
        if ((pad->type == PAD_ID_DIGITAL) || (pad->type == PAD_ID_ANALOG_STICK) || (pad->type == PAD_ID_ANALOG)) {
            if (!(pad->btn & PAD_LEFT))
                cmd->sidemove -= speed*cl_sidespeed.value;
            if (!(pad->btn & PAD_RIGHT))
                cmd->sidemove += speed*cl_sidespeed.value;
            if (!(pad->btn & PAD_UP))
                cmd->forwardmove += speed*cl_forwardspeed.value;
            if (!(pad->btn & PAD_DOWN))
                cmd->forwardmove -= speed*cl_backspeed.value;
        }
    }
}

