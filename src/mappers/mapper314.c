/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
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
 *
 * BMC 42-in-1 "reset switch" type
 */

#include "mapinc.h"
#include "latch.h"

static uint8 regs[4];

static SFORMAT StateRegs[] =
{
	{ regs, 4, "REGS" },
	{ 0 }
};

static void Sync(void) {
	if (regs[0] & 0x80) { /* NROM mode */
		if (regs[1] & 0x80)
			setprg32(0x8000, regs[1] & 0x3F);
		else {
			int bank = ((regs[1] & 0x3F) << 1) | ((regs[1] >> 6) & 1);
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		}
	} else { /* UNROM mode */
		setprg16(0x8000, (regs[1] << 1) | (latch.data & 7));
		setprg16(0xC000, (regs[1] << 1) | 7);
	}
	if (regs[0] & 0x20)
		setmirror(MI_H);
	else
		setmirror(MI_V);
	setchr8((regs[2] << 2) | ((regs[0] >> 1) & 3));
}

static DECLFW(M314WriteLo) {
	A &= 3;
	if (A == 3)
		A = 1; /* K-42001's "Aladdin III" */
	regs[A & 3] = V;
	Sync();
}

static void M314Power(void) {
	regs[0] = 0x80;
	regs[1] = 0x43;
	regs[2] = regs[3] = 0;
	Latch_Power();
	SetWriteHandler(0x5000, 0x5FFF, M314WriteLo);
}

static void M314Reset(void) {
	/* Reset returns to menu */
	regs[0] = 0x80;
	regs[1] = 0x43;
	regs[2] = regs[3] = 0;
	Sync();

}

void Mapper314_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 0);
	info->Power = M314Power;
	info->Reset = M314Reset;
	AddExState(&StateRegs, ~0, 0, 0);
}