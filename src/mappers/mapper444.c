/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
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

/* NC7000M PCB, with incorrect UNIF MAPR BS-110 due to a mix-up. Submapper bits 0 and 1. denote the setting of two
 * solder pads that configure CHR banking. */
/* NC8000M PCB, indicated by submapper bit 2. */

#include "mapinc.h"
#include "mmc3.h"

static uint8 reg;
static uint8 pads;
static uint8 dip;

static void M444PW(uint32 A, uint8 V) {
	uint16 mask = ((pads & 0x04) && (reg & 0x02)) ? 0x1F : 0x0F;
	uint16 base = reg << 4;

	if (reg & 0x04) {
		if (reg & 0x08) {
			setprg8(0x8000, (base & ~mask) | (mmc3.reg[6] & mask));
			setprg8(0xA000, (base & ~mask) | (mmc3.reg[7] & mask));
			setprg8(0xC000, (base & ~mask) | (mmc3.reg[6] & mask));
			setprg8(0xE000, (base & ~mask) | (mmc3.reg[7] & mask));
		} else {
			setprg8(0x8000, (base & ~mask) | ((mmc3.reg[6] & ~0x02) & mask));
			setprg8(0xA000, (base & ~mask) | ((mmc3.reg[7] & ~0x02) & mask));
			setprg8(0xC000, (base & ~mask) | ((mmc3.reg[6] |  0x02) & mask));
			setprg8(0xE000, (base & ~mask) | ((mmc3.reg[7] |  0x02) & mask));
		}
	} else {
		setprg8(A, (base & ~mask) | (V & mask));
	}
}

static void M444CW(uint32 A, uint8 V) {
	uint16 mask = (pads & 1) ? 0xFF : 0x7F;
	uint16 base = ((reg << 7) & ((pads & 1) ? 0x00 : 0x80)) | ((reg << ((pads & 2) ? 4 : 7)) & 0x100);

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFR(M444Read) {
	return (reg & 0x0C) == 0x08 ? dip : CartBR(A);
}

static DECLFW(M444Write) {
	reg = A & 0xFF;
	MMC3_FixPRG();
	MMC3_FixCHR();
}

static void M444Reset(void) {
	dip++;
	dip &= 3;
	reg = 0;
	MMC3_Reset();
}

static void M444Power(void) {
	dip = 0;
	reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, M444Write);
	SetReadHandler(0x8000, 0xFFFF, M444Read);
}

void Mapper444_Init(CartInfo *info) {
	pads = info->submapper; /* UNIF represents submapper 0 */
	MMC3_Init(info, 0, 0);
	MMC3_cwrap = M444CW;
	MMC3_pwrap = M444PW;
	info->Power = M444Power;
	info->Reset = M444Reset;
	AddExState(&reg, 1, 0, "EXPR");
	AddExState(&dip, 1, 0, "DIPS");
}