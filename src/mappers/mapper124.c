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
 *
 */

/* iNES mapper 124 is assigned to the Super Game Mega Type III pirate arcade board. */

#include "mapinc.h"
#include "mmc1.h"
#include "mmc3.h"
#include "latch.h"

#define MAPPER_UNROM 0
#define MAPPER_AMROM 1
#define MAPPER_MMC1  2
#define MAPPER_MMC3  3

static uint8 reg[2];
static uint8 mapper;
static uint8 dipsw;

static uint8 *WRAM = NULL;
static uint32 WRAMSIZE;

static uint8 *CHRRAM = NULL;
static uint32 CHRRAMSIZE;

static uint8 audioEnable;
static uint8 ram[0x800];

static SFORMAT StateRegs[] = {
	{ reg, 2, "MODE" },
	{ &mapper, 1, "MAPR" },
	{ 0 }
};

static uint32 GetPRGBase(void) {
	return ((reg[1] << 4) & 0x1F0);
}

static uint32 GetCHRBase(void) {
	return ((reg[0] << 7) & 0x780);
}

static void M124PW_mmc1(uint32 A, uint8 V) {
	setprg16(A, (GetPRGBase() >> 1) | (V & 0x07));
}

static void M124CW_mmc1(uint32 A, uint8 V) {
	setchr4(A, (GetCHRBase() >> 2) | (V & 0x1F));
}

static void M124PW_mmc3(uint32 A, uint8 V) {
	uint16 mask = (reg[1] & 0x20) ? 0x0F : 0x1F;
	setprg8(A, GetPRGBase() | (V & mask));
}

static void M124CW_mmc3(uint32 A, uint8 V) {
	uint16 mask = (reg[0] & 0x40) ? 0x7F : 0xFF;
	setchr1(A, GetCHRBase() | (V & mask));
}

void Sync(void) {
	switch (mapper) {
	case MAPPER_UNROM:
		setprg16(0x8000, (GetPRGBase() >> 1) | (latch.data & 0x07));
		setprg16(0xC000, (GetPRGBase() >> 1) | 0x07);
		setchr8(0);
		setmirror(MI_V);
		break;
	case MAPPER_AMROM:
		setprg32(0x8000, (GetPRGBase() >> 2) | (latch.data & 0x07));
		setchr8(0);
		setmirror(MI_0 + ((latch.data & 0x10) >> 4));
		break;
	case MAPPER_MMC1:
		setprg16r(0x10, 0x6000, 0);
		MMC1_FixPRG();
		MMC1_FixCHR();
		MMC1_FixMIR();
		break;
	case MAPPER_MMC3:
		setprg16r(0x10, 0x6000, 0);
		MMC3_FixPRG();
		MMC3_FixCHR();
		MMC3_FixMIR();
		break;
	}
	setprg4(0x5000, 0x380 + 0x05);
	if (reg[1] & 0x20) {
		setprg4(0x6000, 0x380 + 0x06);
		setprg4(0x7000, 0x380 + 0x07);
	}
	if (!(reg[1] & 0x80)) {
		setprg4(0x8000, 0x380 + 0x08);
		setprg4(0x9000, 0x380 + 0x09);
		setprg4(0xA000, 0x380 + 0x0A);
		setprg4(0xB000, 0x380 + 0x0B);
		setprg4(0xC000, 0x380 + 0x0C);
		setprg4(0xD000, 0x380 + 0x0D);
		setprg4(0xE000, 0x380 + 0x0E);
		setprg4(0xF000, 0x380 + 0x0F);
	}
	if (!(reg[1] & 0x40)) {
		setchr8r(0x10, 0);
	}
}

void applyMode(void) {
	mapper = (reg[0] >> 4) & 0x03;
}

extern int coinon;
static DECLFR(M124ReadCoinDIP) {
	if ((A & 0x0F) == 0x0F) {
		/* TODO: Hack to use VS' coin insert map to use as mappers coin insert trigger */
		return ((coinon ? 0x80 : 0x00) | (dipsw & 0x7F));
	}
	return X.DB;
}

static DECLFW(M124WriteReg) {
	if (A & 0x10) {
		audioEnable = V;
	} else {
		reg[A & 0x01] = V;
		applyMode();
		Sync();
	}
}

static DECLFW(M124Write) {
	switch (mapper) {
	case MAPPER_UNROM:
	case MAPPER_AMROM:
		Latch_Write(A, V);
		break;
	case MAPPER_MMC3:
		MMC3_Write(A, V);
		break;
	case MAPPER_MMC1:
		MMC1_Write(A, V);
		break;
	}
}

static DECLFR(M124ReadRAM) {
	return ram[A - 0x800];
}

static DECLFW(M124WriteRAM) {
	ram[A - 0x800] = V;
}

static void M124Close(void) {
	if (WRAM) {
		FCEU_gfree(WRAM);
		WRAM = NULL;
	}
	if (CHRRAM) {
		FCEU_gfree(CHRRAM);
		CHRRAM = NULL;
	}
}

static void M124Reset(void) {
	dipsw++;
	Sync();
}

static void M124Power(void) {
	memset(ram, 0, sizeof(ram));
	memset(reg, 0, sizeof(reg));
	mapper = 0;
	dipsw = 0;

	MMC1_Reset();
	MMC3_Reset();
	Latch_RegReset();

	SetReadHandler(0x0800, 0x0FFF, M124ReadRAM);
	SetWriteHandler(0x0800, 0x0FFF, M124WriteRAM);

	SetReadHandler(0x4F00, 0x4FFF, M124ReadCoinDIP);

	SetReadHandler(0x5000, 0xFFFF, CartBR);
	SetWriteHandler(0x5000, 0x5FFF, M124WriteReg);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetWriteHandler(0x8000, 0xFFFF, M124Write);

	applyMode();
	Sync();
}

static void M124HBIRQHook(void) {
	if (mapper == MAPPER_MMC3) {
		MMC3_IRQHBHook();
	}
}

static void StateRestore(int version) {
	applyMode();
	Sync();
}

void Mapper124_Init(CartInfo *info) {
	MMC1_Init(info, FALSE, FALSE);
	MMC1_pwrap = M124PW_mmc1;
	MMC1_cwrap = M124CW_mmc1;
	mmc1_type = MMC1A;

	MMC3_Init(info, FALSE, FALSE);
	MMC3_pwrap = M124PW_mmc3;
	MMC3_cwrap = M124CW_mmc3;

	Latch_Init(info, Sync, NULL, FALSE, TRUE);

	info->Power = M124Power;
	info->Reset = M124Reset;
	info->Close = M124Close;
	GameHBIRQHook = M124HBIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8 *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, TRUE);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");

	CHRRAMSIZE = 8192;
	CHRRAM = (uint8 *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, TRUE);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
}
