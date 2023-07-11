/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
 *  Copyright (C) 2002 Xodnizel
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
 */

#include "mapinc.h"
#include "mmc4.h"

static uint8 *WRAM = NULL;
static uint32 WRAMSIZE;

void (*MMC4_pwrap)(uint32 A, uint8 V);
void (*MMC4_cwrap)(uint32 A, uint8 V);
void (*MMC4_mwrap)(uint8 V);

MMC4 mmc4;

static SFORMAT StateRegs[] =
{
	{ mmc4.chr,   4, "CREG" },
	{ mmc4.latch, 2, "PPUL" },
	{ &mmc4.prg,  1, "PREG" },
	{ &mmc4.mirr, 1, "MIRR" },
	{ 0 }
};

static void GENPWRAP(uint32 A, uint8 V) {
	setprg16(A, V);
}

static void GENCWRAP(uint32 A, uint8 V) {
	setchr4(A, V);
}

static void GENMWRAP(uint8 V) {
	mmc4.mirr = V;
	setmirror((mmc4.mirr & 1) ^ 1);
}

void MMC4_FixPRG(void) {
	MMC4_pwrap(0x8000, mmc4.prg);
	MMC4_pwrap(0xC000, ~0);
}

void MMC4_FixCHR(void) {
	MMC4_cwrap(0x0000, mmc4.chr[mmc4.latch[0] | 0]);
	MMC4_cwrap(0x1000, mmc4.chr[mmc4.latch[1] | 2]);

	if (MMC4_mwrap) {
		MMC4_mwrap(mmc4.mirr);
	}
}

DECLFW(MMC4_Write) {
	switch (A & 0xF000) {
	case 0xA000:
		mmc4.prg = V;
		MMC4_FixPRG();
		break;
	case 0xB000:
	case 0xC000:
	case 0xD000:
	case 0xE000:
		mmc4.chr[(A - 0xB000) >> 12] = V;
		MMC4_FixCHR();
		break;
	case 0xF000:
		if (MMC4_mwrap) {
			MMC4_mwrap(V);
		}
		break;
	}
}

static void MMC4PPUHook(uint32 A) {
	uint8 bank = (A >> 12) & 0x01;
	if ((A & 0x2000) || (((A & 0xFF0) != 0xFD0) && ((A & 0xFF0) != 0xFE0))) {
		return;
	}
	mmc4.latch[bank] = (A >> 5) & 0x01;
	MMC4_FixCHR();
}

void GenMMC4Reset(void) {
	mmc4.prg = mmc4.mirr = 0;
	mmc4.latch[0] = mmc4.latch[1] = 0;
	MMC4_FixPRG();
	MMC4_FixCHR();
}

void GenMMC4Power(void) {
	GenMMC4Reset();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0xA000, 0xFFFF, MMC4_Write);
	if (WRAMSIZE) {
		setprg8r(0x10, 0x6000, 0);
		SetReadHandler(0x6000, 0x7FFF, CartBR);
		SetWriteHandler(0x6000, 0x7FFF, CartBW);
		FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
	}
}

void GenMMC4Restore(int version) {
	MMC4_FixPRG();
	MMC4_FixCHR();
}

void GenMMC4Close(void) {
	if (WRAM) {
		FCEU_gfree(WRAM);
	}
	WRAM = NULL;
}

void MMC4_Init(CartInfo *info, int wram, int battery) {
	MMC4_pwrap = GENPWRAP;
	MMC4_cwrap = GENCWRAP;
	MMC4_mwrap = GENMWRAP;

	info->Power = GenMMC4Power;
	info->Close = GenMMC4Close;
	PPU_hook = MMC4PPUHook;

	GameStateRestore = GenMMC4Restore;
	AddExState(&StateRegs, ~0, 0, 0);

	if (wram) {
		WRAMSIZE = wram * 1024;
		WRAM = (uint8*)FCEU_gmalloc(WRAMSIZE);
		SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");
		if (battery) {
			info->SaveGame[0] = WRAM;
			info->SaveGameLen[0] = WRAMSIZE;
		}
	}
}
