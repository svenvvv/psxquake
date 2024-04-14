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

void IN_Commands(void)
{
    PADTYPE * pad = (PADTYPE *)padbuf[0];

    if (pad->stat == 0) {
        if ((pad->type == PAD_ID_DIGITAL) || (pad->type == PAD_ID_ANALOG_STICK) || (pad->type == PAD_ID_ANALOG)) {
            Key_Event (K_UPARROW, !(pad->btn & PAD_UP));
            Key_Event (K_DOWNARROW, !(pad->btn & PAD_DOWN));
            Key_Event (K_LEFTARROW, !(pad->btn & PAD_LEFT));
            Key_Event (K_RIGHTARROW, !(pad->btn & PAD_RIGHT));
            Key_Event (K_ENTER, !(pad->btn & PAD_CROSS));

            Key_Event (K_ESCAPE, !(pad->btn & PAD_START));
        }
    }
}

void IN_Move(usercmd_t * cmd)
{
    printf("IN_Move TODO\n");
}

