/* FCE Ultra - NES/Famicom Emulator
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

#include "mapinc.h"

static uint8 chr[2];
static SFORMAT StateRegs[] =
{
	{ chr, 2, "CHR" },
	{ 0 }
};

static void Sync(void) {
	setprg32(0x8000, 0);
	setchr4(0x0000, chr[0]);
	setchr4(0x1000, chr[1]);
}

static DECLFW(UNLKS7058Write) {
	chr[A & 1] = V;
	Sync();
}

static void UNLKS7058Power(void) {
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, UNLKS7058Write);
}

static void StateRestore(int version) {
	Sync();
}

void UNLKS7058_Init(CartInfo *info) {
	info->Power = UNLKS7058Power;
	GameStateRestore = StateRestore;
	AddExState(&StateRegs, ~0, 0, 0);
}