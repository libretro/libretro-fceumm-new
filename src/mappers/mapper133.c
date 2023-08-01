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

/* iNES Mapper 133 - Sachen 3009 */

#include "mapinc.h"

static uint8 reg = 0;

static SFORMAT StateRegs[] = {
	{ &reg, 1,  "REGS" },
	{ 0 }
};

static void Sync(void) {
	setprg32(0x8000, (reg >> 2) & 0x01);
	setchr8(reg & 0x03);
}

static DECLFW(M133Write) {
	if (A & 0x100) {
		reg = V;
		Sync();
	}
}

static void M133Power(void) {
	reg = 0;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x4100, 0x5FFF, M133Write);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper133_Init(CartInfo *info) {
	info->Power = M133Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
