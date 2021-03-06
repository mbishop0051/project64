/*
 * RSP Compiler plug in for Project64 (A Nintendo 64 emulator).
 *
 * (c) Copyright 2001 jabo (jabo@emulation64.com) and
 * zilmar (zilmar@emulation64.com)
 *
 * pj64 homepage: www.pj64.net
 * 
 * Permission to use, copy, modify and distribute Project64 in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Project64 is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Project64 or software derived from Project64.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so if they want them.
 *
 */

#include <stdio.h>
#include <string.h>
#include <Common/stdtypes.h>

#include "Rsp.h"
#include "RSP Registers.h"
#include "memory.h"

// #define RSP_SAFE_DMA /* unoptimized dma transfers */

void SP_DMA_READ (void)
{
    uint32_t i, j, Length, Skip, Count, End, addr;
    uint8_t *Dest, *Source;

    addr = (*RSPInfo.SP_DRAM_ADDR_REG) & 0x00FFFFFF;

	if (addr > 0x7FFFFF)
	{
        DisplayError("SP DMA READ\nSP_DRAM_ADDR_REG not in RDRam space");
		return;
	}
	
	if ((*RSPInfo.SP_RD_LEN_REG & 0xFFF) + 1  + (*RSPInfo.SP_MEM_ADDR_REG & 0xFFF) > 0x1000)
	{
        DisplayError("SP DMA READ\ncould not fit copy in memory segment");
		return;
	}

	Length = ((*RSPInfo.SP_RD_LEN_REG & 0xFFF) | 7) + 1;
	Skip = (*RSPInfo.SP_RD_LEN_REG >> 20) + Length;
	Count = ((*RSPInfo.SP_RD_LEN_REG >> 12) & 0xFF)  + 1;
	End = ((*RSPInfo.SP_MEM_ADDR_REG & 0x0FFF) & ~7) + (((Count - 1) * Skip) + Length);

	if ((*RSPInfo.SP_MEM_ADDR_REG & 0x1000) != 0)
	{
		Dest = RSPInfo.IMEM + ((*RSPInfo.SP_MEM_ADDR_REG & 0x0FFF) & ~7);
	}
	else
	{
		Dest = RSPInfo.DMEM + ((*RSPInfo.SP_MEM_ADDR_REG & 0x0FFF) & ~7);
	}
	Source = RSPInfo.RDRAM + (addr & ~7);

#if defined(RSP_SAFE_DMA)
	for (j = 0 ; j < Count; j++)
	{
		for (i = 0 ; i < Length; i++)
		{
			*(uint8_t *)(((size_t)Dest + j * Length + i) ^ 3) = *(uint8_t *)(((size_t)Source + j * Skip + i) ^ 3);
		}
	}
#else
	if ((Skip & 0x3) == 0)
	{
		for (j = 0; j < Count; j++)
		{
			memcpy(Dest, Source, Length);
			Source += Skip;
			Dest += Length;
		}
	}
	else
	{
		for (j = 0 ; j < Count; j++)
		{
			for (i = 0 ; i < Length; i++)
			{
				*(uint8_t *)(((size_t)Dest + i) ^ 3) = *(uint8_t *)(((size_t)Source + i) ^ 3);
			}
			Source += Skip;
			Dest += Length;
		}
	}
#endif

	/* FIXME: could this be a problem DMEM to IMEM (?) */
	if (CPUCore == RecompilerCPU && (*RSPInfo.SP_MEM_ADDR_REG & 0x1000) != 0)
	{
		SetJumpTable(End);
	}

	*RSPInfo.SP_DMA_BUSY_REG = 0;
	*RSPInfo.SP_STATUS_REG  &= ~SP_STATUS_DMA_BUSY;
}

void SP_DMA_WRITE (void)
{
    uint32_t i, j, Length, Skip, Count, addr;
    uint8_t *Dest, *Source;

    addr = (*RSPInfo.SP_DRAM_ADDR_REG) & 0x00FFFFFF;

	if (addr > 0x7FFFFF)
	{
        DisplayError("SP DMA WRITE\nSP_DRAM_ADDR_REG not in RDRam space");
		return;
	}

	if ((*RSPInfo.SP_WR_LEN_REG & 0xFFF) + 1  + (*RSPInfo.SP_MEM_ADDR_REG & 0xFFF) > 0x1000)
	{
        DisplayError("SP DMA WRITE\ncould not fit copy in memory segment");
		return;
	}

	Length = ((*RSPInfo.SP_WR_LEN_REG & 0xFFF) | 7) + 1;
	Skip = (*RSPInfo.SP_WR_LEN_REG >> 20) + Length;
	Count = ((*RSPInfo.SP_WR_LEN_REG >> 12) & 0xFF)  + 1;
	Dest = RSPInfo.RDRAM + (addr & ~7);
	Source = RSPInfo.DMEM + ((*RSPInfo.SP_MEM_ADDR_REG & 0x1FFF) & ~7);

#if defined(RSP_SAFE_DMA)
	for (j = 0 ; j < Count; j++)
	{
		for (i = 0 ; i < Length; i++)
		{
			*(uint8_t *)(((size_t)Dest + j * Skip + i) ^ 3) = *(uint8_t *)(((size_t)Source + j * Length + i) ^ 3);
		}
	}
#else
	if ((Skip & 0x3) == 0)
	{
		for (j = 0; j < Count; j++)
		{
			memcpy(Dest, Source, Length);
			Source += Length;
			Dest += Skip;
		}
	}
	else
	{
		for (j = 0 ; j < Count; j++)
		{
			for (i = 0 ; i < Length; i++)
			{
				*(uint8_t *)(((size_t)Dest + i) ^ 3) = *(uint8_t *)(((size_t)Source + i) ^ 3);
			}
			Source += Length;
			Dest += Skip;
		}
	}
#endif
	*RSPInfo.SP_DMA_BUSY_REG = 0;
	*RSPInfo.SP_STATUS_REG  &= ~SP_STATUS_DMA_BUSY;
}
