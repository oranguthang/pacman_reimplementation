#ifndef PACMAN_COMPAT_H
#define PACMAN_COMPAT_H
/*
 * Compatibility shim: maps pacman_src register names and missing functions
 * to nes-starter-kit equivalents.
 */

#include "source/c/neslib.h"

/* PPU hardware registers (direct memory-mapped I/O) */
#define PPU_STATUS_REG  (*(volatile unsigned char*)0x2002)
#define PPU_CTRL_REG    (*(volatile unsigned char*)0x2000)
#define PPU_SCROLL_REG  (*(volatile unsigned char*)0x2005)
#define PPU_ADDR_REG    (*(volatile unsigned char*)0x2006)
#define PPU_DATA_REG    (*(volatile unsigned char*)0x2007)
#define OAM_ADDR_REG    (*(volatile unsigned char*)0x2003)
#define OAM_DATA_REG    (*(volatile unsigned char*)0x2004)

/* ppu_wait_vblank: nes-starter-kit uses ppu_wait_nmi for the same purpose */
#define ppu_wait_vblank() ppu_wait_nmi()

/* ppu_clear_nt: fill an entire nametable (0x400 bytes) with a tile value */
#define ppu_clear_nt(addr, tile) do { vram_adr(addr); vram_fill((tile), 0x400); } while(0)

#endif /* PACMAN_COMPAT_H */
