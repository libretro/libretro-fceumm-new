/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* iNES Mapper 139 - Sachen 8259C */

#include "mapinc.h"

static uint8 cmd;
static uint8 reg[8];

static void Sync(void) {
	setprg32(0x8000, reg[5]);
	setchr2(0x0000, ((((reg[4] << 3) | (reg[(reg[7] & 0x01) ? 0 : 0] & 0x07)) << 2) | 0));
	setchr2(0x0800, ((((reg[4] << 3) | (reg[(reg[7] & 0x01) ? 0 : 1] & 0x07)) << 2) | 1));
	setchr2(0x1000, ((((reg[4] << 3) | (reg[(reg[7] & 0x01) ? 0 : 2] & 0x07)) << 2) | 2));
	setchr2(0x1800, ((((reg[4] << 3) | (reg[(reg[7] & 0x01) ? 0 : 3] & 0x07)) << 2) | 3));
	switch (reg[7] & 0x07) {
	case 0:  setmirrorw(0, 0, 0, 1); break;
	case 2:  setmirror(MI_H); break;
    default: setmirror(MI_V); break;
	case 6:  setmirror(MI_0); break;
	}
}

static DECLFW(M139Write) {
	if ((A & 0x4000) && (A & 0x100)) {
		if (A & 0x01) {
			reg[cmd & 0x07] = V;
			Sync();
		} else {
			cmd = V;
		}
	}
}

static void M139Reset(void) {
	cmd = 0;
	reg[0] = reg[1] = reg[2] = reg[3] = 0;
	reg[4] = reg[5] = reg[6] = reg[7] = 0;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x4100, 0x7FFF, M139Write);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper139_Init(CartInfo *info) {
	info->Power = M139Reset;
	GameStateRestore = StateRestore;
	AddExState(reg, 8, 0, "REGS");
	AddExState(&cmd, 1, 0, "CMD");
}
