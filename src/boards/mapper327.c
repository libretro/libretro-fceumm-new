/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *	Copyright (C) 2015 Cluster
 *	http://clusterrr.com
 *	clusterrr@clusterrr.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA	02110-1301	USA
 */
 
/*
    NES 2.0 mapper 327 is used for a 6-in-1 multicart. Its UNIF board name is BMC-10-24-C-A1.

	MMC3-based multicart mapper with CHR RAM, CHR ROM and PRG RAM
	
	$6000-7FFF:	A~[011xxxxx xxMRSBBB]	Multicart reg
		This register can only be written to if PRG-RAM is enabled and writable (see $A001)
		and BBB = 000 (power on state)
	
	BBB = CHR+PRG block select bits (A19, A18, A17 for both PRG and CHR)
	S = PRG block size (0=256k	 1=128k)
	R = CHR mode (0=CHR ROM	 1=CHR RAM)
	M = CHR block size (0=256k	 1=128k)
		ignored when S is 0 for some reason
		
 Example Game:
 --------------------------
 6 in 1 multicart (SMB3, TMNT2, Contra, Ninja Cat, Ninja Crusaders, Rainbow Islands 2)
*/

#include "mapinc.h"
#include "mmc3.h"

static uint8 *CHRRAM = NULL;
static uint32 CHRRAMSize;

static void M327PW(uint32 A, uint8 V) {
	if (mmc3.expregs[0] & 8)
		setprg8(A, (V & 0x1F) | ((mmc3.expregs[0] & 7) << 4));
	else
		setprg8(A, (V & 0x0F) | ((mmc3.expregs[0] & 7) << 4));
}

static void M327CW(uint32 A, uint8 V) {
	if ((mmc3.expregs[0] >> 4) & 1)
		setchr1r(0x10, A, V);
	else if (mmc3.expregs[0] & 0x20)
		setchr1(A, V | ((mmc3.expregs[0] & 7) << 7));
	else
		setchr1(A, (V & 0x7F) | ((mmc3.expregs[0] & 7) << 7));
}

static DECLFW(M327Write) {
	if (MMC3CanWriteToWRAM()) {
		CartBW(A, V);
		if ((mmc3.expregs[0] & 7) == 0) {
			mmc3.expregs[0] = A & 0x3F;
			FixMMC3PRG(mmc3.cmd);
			FixMMC3CHR(mmc3.cmd);
		}
	}
}

static void M327Reset(void) {
	mmc3.expregs[0] = 0;
	MMC3RegReset();
}

static void M327Power(void) {
	mmc3.expregs[0] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M327Write);
}

static void M327Close(void) {
	GenMMC3Close();
	if (CHRRAM)
		FCEU_gfree(CHRRAM);
	CHRRAM = NULL;
}

void Mapper327_Init(CartInfo *info) {
	GenMMC3_Init(info, 256, 256, 8, 0);
	CHRRAMSize = 8192;
	CHRRAM = (uint8*)FCEU_gmalloc(CHRRAMSize);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSize, 1);
	AddExState(CHRRAM, CHRRAMSize, 0, "CHRR");
	mmc3.pwrap = M327PW;
	mmc3.cwrap = M327CW;
	info->Power = M327Power;
	info->Reset = M327Reset;
	info->Close = M327Close;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}