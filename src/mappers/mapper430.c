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

#include "mapinc.h"
#include "mmc3.h"

static uint8 reg;

static void M430PW(uint32 A, uint8 V) {
	if (reg & 0x08) {
		setprg8(0x8000, ((reg << 4) & 0x30) | ((mmc3.reg[6] & ~2) & 0x0F));
		setprg8(0xA000, ((reg << 4) & 0x30) | ((mmc3.reg[7] & ~2) & 0x0F));
		setprg8(0xC000, ((reg << 4) & 0x30) | ((mmc3.reg[6] |  2) & 0x0F));
		setprg8(0xE000, ((reg << 4) & 0x30) | ((mmc3.reg[7] |  2) & 0x0F));
	} else {
		setprg8(A, ((reg << 4) & 0x30) | (V & 0x0F));
	}
}

static void M430CW(uint32 A, uint8 V) {
	uint8 mask = (reg & 0x04) ? 0x7F : 0xFF;

	setchr1(A, ((reg << 6) & ~mask) | (V & mask));
}

static DECLFW(M430Write) {
	if (MMC3_WramIsWritable()) {
	    reg = A & 0xFF;
	    MMC3_FixPRG();        
	    MMC3_FixCHR();
    }
}

static void M430Reset(void) {
	reg = 0;
	MMC3_Reset();
}

static void M430Power(void) {
	reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, M430Write);
}

void Mapper430_Init(CartInfo *info) {
	MMC3_Init(info, 0, 0);
	MMC3_cwrap = M430CW;
	MMC3_pwrap = M430PW;
	info->Reset = M430Reset;
	info->Power = M430Power;
	AddExState(&reg, 1, 0, "EXPR");
}
