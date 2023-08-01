/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2011 CaH4e3
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
 *
 * SL12 Protected 3-in-1 mapper hardware (VRC2, MMC3, MMC1)
 * the same as 603-5052 board (TODO: add reading registers, merge)
 * SL1632 2-in-1 protected board, similar to SL12 (TODO: find difference)
 *
 * Known PCB:
 *
 * Garou Densetsu Special (G0904.PCB, Huang-1, GAL dip: W conf.)
 * Kart Fighter (008, Huang-1, GAL dip: W conf.)
 * Somari (008, C5052-13, GAL dip: P conf., GK2-P/GK2-V maskroms)
 * Somari (008, Huang-1, GAL dip: W conf., GK1-P/GK1-V maskroms)
 * AV Mei Shao Nv Zhan Shi (aka AV Pretty Girl Fighting) (SL-12 PCB, Hunag-1, GAL dip: unk conf. SL-11A/SL-11B maskroms)
 * Samurai Spirits (Full version) (Huang-1, GAL dip: unk conf. GS-2A/GS-4A maskroms)
 * Contra Fighter (603-5052 PCB, C5052-3, GAL dip: unk conf. SC603-A/SCB603-B maskroms)
 *
 */

#include "mapinc.h"
#include "vrc2and4.h"
#include "mmc3.h"
#include "mmc1.h"

static uint8 mode = 0;
static uint8 game = 0;

static SFORMAT StateRegs[] = {
	{ &mode, 1, "MODE"},
	{ &game, 1, "GAME"},
	{ 0 }
};

static uint32 GetPRGMask(void) {
	return ((iNESCart.submapper != 3) ? 0x3F : (game ? 0x0F : 0x1F));
}

static uint32 GetPRGBase(void) {
	return (game ? (game + 1) * 0x10 : 0);
}

static uint32 GetCHRMask(void) {
	return (game ? 0x7F : 0xFF);
}

static uint32 GetCHRBase(void) {
	return (game ? (game + 1) * 0x80 : 0);
}

static void M116PW_vrc2(uint32 A, uint8 V) {
	setprg8(A, GetPRGBase() | (V & GetPRGMask()));
}

static void M116PW_mmc3(uint32 A, uint8 V) {
	setprg8(A, GetPRGBase() | (V & GetPRGMask()));
}

static void M116PW_mmc1(uint32 A, uint8 V) {
	if (iNESCart.submapper == 2) {
		setprg16(A, V >> 1);
	} else {
		setprg16(A, (GetPRGBase() >> 1) | (V & (GetPRGMask() >> 1)));
	}
}

static void M116CW_vrc2(uint32 A, uint32 V) {
	setchr1(A, ((mode << 6) & 0x100) | GetCHRBase() | (V & GetCHRMask()));
}

static void M116CW_mmc3(uint32 A, uint8 V) {
	setchr1(A, ((mode << 6) & 0x100) | GetCHRBase() | (V & GetCHRMask()));
}

static void M116CW_mmc1(uint32 A, uint8 V) {
	setchr4(A, (GetCHRBase() >> 2) | (V & (GetCHRMask() >> 2)));
}

static void Sync(void) {
	switch (mode & 0x03) {
	case 0:
		VRC24_FixPRG();
		VRC24_FixCHR();
		VRC24_FixMIR();
		break;
	case 1:
		MMC3_FixPRG();
		MMC3_FixCHR();
		MMC3_FixMIR();
		break;
	default:
		MMC1_FixPRG();
		MMC1_FixCHR();
		MMC1_FixMIR();
		break;
	}
}

static DECLFW(M116ModeWrite) {
	if (A & 0x100) {
		mode = V;
		switch (mode & 0x03) {
		case 0: break;
		case 1: break;
		default:
			if (iNESCart.submapper != 1) {
				MMC1_Write(0x8000, 0x80);
			}
			break;
		}
		Sync();
	}
}

static DECLFW(M116Write) {
	/*FCEU_printf("%04X:%02X mode:%02x\n",A,V,mode);*/
	switch (mode & 0x03) {
	case 0:  VRC24_Write(A, V); break;
	case 1:  MMC3_Write(A, V); break;
	default: MMC1_Write(A, V); break;
	}
}

static void M116HBIRQ(void) {
	if ((mode & 0x03) == 0x01) {
		MMC3_IRQHBHook();
	}
}

static void StateRestore(int version) {
	Sync();
}

static void M116Reset(void) {
	if (iNESCart.submapper == 3) {
		game = game + 1;
		if (game > 4) {
			game = 0;
		}
	}
	Sync();
}

static void M116Power(void) {
	game = (iNESCart.submapper == 3) ? 4 : 0;
	mode = 1;

	MMC3_Power();
	MMC1_Reset();
	VRC24_Power();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x4100, 0x5FFF, M116ModeWrite);
	SetWriteHandler(0x8000, 0xFFFF, M116Write);

	vrc24.chr[0] = ~0;
	vrc24.chr[1] = ~0;
	vrc24.chr[2] = ~0;
	vrc24.chr[3] = ~0;

	Sync();
}

void Mapper116_Init(CartInfo *info) {
	VRC24_Init(info, VRC2, 0x01, 0x02, FALSE, TRUE);
	VRC24_pwrap = M116PW_vrc2;
	VRC24_cwrap = M116CW_vrc2;

	MMC3_Init(info, FALSE, FALSE);
	MMC3_pwrap = M116PW_mmc3;
	MMC3_cwrap = M116CW_mmc3;

	MMC1_Init(info, FALSE, FALSE);
	MMC1_pwrap = M116PW_mmc1;
	MMC1_cwrap = M116CW_mmc1;
	mmc1_type = MMC1A;

	info->Power = M116Power;
	info->Reset = M116Reset;

	GameHBIRQHook = M116HBIRQ;

	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	/* PRG 128K and CHR 128K is Huang-2 (iNESCart.submapper 2) */
	if (((ROM.prg.size * 16) == 128) && ((ROM.chr.size * 8) == 128)) {
		info->submapper = 2;
	}
}
