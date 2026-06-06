#include "source/c/map/map.h"
#include "source/c/map/load_map.h"
#include "source/c/neslib.h"
#include "source/c/library/bank_helpers.h"
#include "source/c/configuration/game_states.h"
#include "source/c/globals.h"
#include "source/c/configuration/system_constants.h"
#include "graphics/palettes/palettes.config.h"
#include "source/c/graphics/hud.h"
#include "source/c/graphics/fade_animation.h"
#include "source/c/pacman/pellets.h"
#include "source/c/sprites/player.h"
#include "source/c/sprites/sprite_definitions.h"
#include "source/c/sprites/map_sprites.h"
#include "source/c/menus/error.h"

CODE_BANK(PRG_BANK_MAP_LOGIC);

ZEROPAGE_DEF(unsigned char, playerOverworldPosition);
ZEROPAGE_DEF(int, xScrollPosition);
ZEROPAGE_DEF(int, yScrollPosition);

unsigned char currentMap[256];

unsigned char assetTable[0x38];

unsigned char currentMapSpriteData[(16 * MAP_MAX_SPRITES)];

unsigned char currentMapSpritePersistance[64];

unsigned char mapScreenBuffer[0x55];


/* Pac-Man maze: 28 columns × 30 rows, centred at column 2 */
#define MAZE_ROWS        27
#define MAZE_COLS        22
#define MAZE_ROW_OFFSET   2
#define MAZE_COL_OFFSET   0

static const unsigned char pacman_maze_tiles[MAZE_ROWS][MAZE_COLS] = {
    {0x2D,0x1F,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x13,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x1D},
    {0x2D,0x11,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x11,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x11},
    {0x2D,0x11,0x03,0x1F,0x10,0x1D,0x03,0x1F,0x10,0x1D,0x03,0x11,0x03,0x1F,0x10,0x1D,0x03,0x1F,0x10,0x1D,0x03,0x11},
    {0x2D,0x11,0x01,0x11,0x20,0x11,0x03,0x11,0x20,0x11,0x03,0x11,0x03,0x11,0x20,0x11,0x03,0x11,0x20,0x11,0x01,0x11},
    {0x2D,0x11,0x03,0x1E,0x10,0x1C,0x03,0x1E,0x10,0x1C,0x03,0x1A,0x03,0x1E,0x10,0x1C,0x03,0x1E,0x10,0x1C,0x03,0x11},
    {0x2D,0x11,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x11},
    {0x2D,0x11,0x03,0x1F,0x10,0x1D,0x03,0x1B,0x03,0x1F,0x10,0x10,0x10,0x1D,0x03,0x1B,0x03,0x1F,0x10,0x1D,0x03,0x11},
    {0x2D,0x11,0x03,0x1E,0x10,0x1C,0x03,0x11,0x03,0x1E,0x10,0x13,0x10,0x1C,0x03,0x11,0x03,0x1E,0x10,0x1C,0x03,0x11},
    {0x2D,0x11,0x03,0x03,0x03,0x03,0x03,0x11,0x03,0x03,0x03,0x11,0x03,0x03,0x03,0x11,0x03,0x03,0x03,0x03,0x03,0x11},
    {0x2D,0x1E,0x10,0x10,0x10,0x1D,0x03,0x15,0x10,0x18,0x08,0x1A,0x08,0x19,0x10,0x14,0x03,0x1F,0x10,0x10,0x10,0x1C},
    {0x2D,0x20,0x20,0x20,0x20,0x11,0x03,0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x11,0x03,0x11,0x20,0x20,0x20,0x20},
    {0x2D,0x20,0x20,0x20,0x20,0x11,0x03,0x11,0x00,0x1F,0x17,0x2C,0x16,0x1D,0x00,0x11,0x03,0x11,0x20,0x20,0x20,0x20},
    {0x2D,0x22,0x10,0x10,0x10,0x1C,0x03,0x1A,0x00,0x11,0x00,0x00,0x00,0x11,0x00,0x1A,0x03,0x1E,0x10,0x10,0x10,0x21},
    {0x04,0x06,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x11,0x00,0x00,0x00,0x11,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x05},
    {0x2D,0x22,0x10,0x10,0x10,0x1D,0x03,0x1B,0x00,0x1E,0x10,0x10,0x10,0x1C,0x00,0x1B,0x03,0x1F,0x10,0x10,0x10,0x21},
    {0x2D,0x20,0x20,0x20,0x20,0x11,0x03,0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x11,0x03,0x11,0x20,0x20,0x20,0x20},
    {0x2D,0x20,0x20,0x20,0x20,0x11,0x03,0x11,0x00,0x1F,0x10,0x10,0x10,0x1D,0x00,0x11,0x03,0x11,0x20,0x20,0x20,0x20},
    {0x2D,0x1F,0x10,0x10,0x10,0x1C,0x03,0x1A,0x00,0x1E,0x10,0x13,0x10,0x1C,0x00,0x1A,0x03,0x1E,0x10,0x10,0x10,0x1D},
    {0x2D,0x11,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x11,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x11},
    {0x2D,0x11,0x03,0x19,0x10,0x1D,0x03,0x19,0x10,0x18,0x09,0x1A,0x09,0x19,0x10,0x18,0x03,0x1F,0x10,0x18,0x03,0x11},
    {0x2D,0x11,0x01,0x03,0x03,0x11,0x03,0x03,0x03,0x03,0x03,0x00,0x03,0x03,0x03,0x03,0x03,0x11,0x03,0x03,0x01,0x11},
    {0x2D,0x15,0x10,0x1D,0x03,0x11,0x03,0x1B,0x03,0x1F,0x10,0x10,0x10,0x1D,0x03,0x1B,0x03,0x11,0x03,0x1F,0x10,0x14},
    {0x2D,0x15,0x10,0x1C,0x03,0x1A,0x03,0x11,0x03,0x1E,0x10,0x13,0x10,0x1C,0x03,0x11,0x03,0x1A,0x03,0x1E,0x10,0x14},
    {0x2D,0x11,0x03,0x03,0x03,0x03,0x03,0x11,0x03,0x03,0x03,0x11,0x03,0x03,0x03,0x11,0x03,0x03,0x03,0x03,0x03,0x11},
    {0x2D,0x11,0x03,0x19,0x10,0x10,0x10,0x12,0x10,0x18,0x03,0x1A,0x03,0x19,0x10,0x12,0x10,0x10,0x10,0x18,0x03,0x11},
    {0x2D,0x11,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x11},
    {0x2D,0x1E,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x1C}
};

static const unsigned char pacman_round_bg_palette[16] = {
    0x0F, 0x30, 0x0F, 0x06,
    0x0F, 0x11, 0x0F, 0x27,
    0x0F, 0x16, 0x26, 0x06,
    0x0F, 0x19, 0x17, 0x12
};

static const unsigned char pacman_round_sprite_palette[16] = {
    0x0F, 0x27, 0x20, 0x06,
    0x0F, 0x11, 0x20, 0x33,
    0x0F, 0x21, 0x20, 0x21,
    0x0F, 0x09, 0x20, 0x17
};

static const unsigned char pacman_maze_attr_bytes[0x40] = {
    0x55, 0x55, 0x55, 0x55, 0x55, 0x11, 0x00, 0x00,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x11, 0x00, 0x00,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x11, 0x00, 0x00,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x51, 0x50, 0x50,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x11, 0x05, 0x05,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x11, 0x00, 0x00,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55
};

static unsigned char maze_get_bg_tile(unsigned char row, unsigned char col) {
    return pacman_pellets_adjust_maze_tile(row, col, pacman_maze_tiles[row][col]);
}

void init_map(void) {
    /* Gameplay uses maze tiles for BG and the original Pac-Man sprite sheet for OAM. */
    set_chr_bank_0(CHR_BANK_TILES);
    set_chr_bank_1(CHR_BANK_SPRITES);

    pal_bg(pacman_round_bg_palette);
    pal_spr(pacman_round_sprite_palette);

    scroll(0, 0);
    set_mirroring(MIRROR_MODE_HORIZONTAL);
}

// Reusing a few temporary vars for the sprite function below.
#define currentValue tempInt1
#define spritePosition tempChar4
#define spriteDefinitionIndex tempChar5
#define mapSpriteDataIndex tempChar6
#define tempArrayIndex tempInt3

/* Pac-Man: no map sprites to load */
void load_sprites(void) {
    for (i = 0; i < MAP_MAX_SPRITES; ++i) {
        currentMapSpriteData[(i << MAP_SPRITE_DATA_SHIFT) + MAP_SPRITE_DATA_POS_TYPE] = SPRITE_TYPE_OFFSCREEN;
    }
    oam_clear();
}

// Clears the asset table. Set containsHud to 1 to set the HUD bytes to use palette 4 (will break the coloring logic if you use the
// last few rows for the map.)
void clear_asset_table(containsHud) {
    // Loop over assetTable to clear it out. 
    for (i = 0; i != sizeof(assetTable) - 8; ++i) {
        assetTable[i] = 0x00;
    }
    // The last row of the asset table uses the 4th palette to show the HUD correctly.
    for (; i != sizeof(assetTable); ++i) {
        assetTable[i] = containsHud == 0 ? 0x00 : 0xff;
    }
}

// Clears the asset table like we do above, but leaves the first row (top *half* of the asset table) blank.
// Used for proper scrolling animation, since we end up flip-flopping on which row we're on during the scrolling up animation.
void clear_asset_table_skip_top(void) {
    clear_asset_table(0);
    return;
    // Loop over assetTable to clear it out. 
    for (i = 0; i != sizeof(assetTable) - 16; ++i) {
        assetTable[i] = 0x00;
    }
    for (; i != sizeof(assetTable) - 8; ++i) {
        assetTable[i] = assetTable[i] & 0xf0;
    }
    // The last row of the asset table uses the 4th palette to show the HUD correctly.
    for (; i != sizeof(assetTable); ++i) {
        assetTable[i] = 0x00;
    }
}

// Loads the assets from assetTable (for the row *ending* with j) into mapScreenBuffer
// at tempArrayIndex. 
void load_palette_to_map_screen_buffer(int attributeTableAdr) {
    mapScreenBuffer[tempArrayIndex++] = MSB(attributeTableAdr + j - 7) | NT_UPD_HORZ;
    mapScreenBuffer[tempArrayIndex++] = LSB(attributeTableAdr + j - 7);
    mapScreenBuffer[tempArrayIndex++] = 8;

    // Using an unrolled loop to save a bit of RAM - not like we need it really.
    mapScreenBuffer[tempArrayIndex++] = assetTable[j-7];
    mapScreenBuffer[tempArrayIndex++] = assetTable[j-6];
    mapScreenBuffer[tempArrayIndex++] = assetTable[j-5];
    mapScreenBuffer[tempArrayIndex++] = assetTable[j-4];
    mapScreenBuffer[tempArrayIndex++] = assetTable[j-3];
    mapScreenBuffer[tempArrayIndex++] = assetTable[j-2];
    mapScreenBuffer[tempArrayIndex++] = assetTable[j-1];
    mapScreenBuffer[tempArrayIndex++] = assetTable[j];
    mapScreenBuffer[tempArrayIndex++] = NT_UPD_EOF;
}

        // Now based on where we are in the map, shift them appropriately.
        // This builds up the palette bytes - which comprise of 2 bits per 16x16 tile. It's a bit confusing...
void update_asset_table_based_on_current_value(unsigned char reverseAttributes) {
    if ((i & 0x01) == 0) {
        // Even/left
        if (((i >> 4) & 0x01) == reverseAttributes) {
            // top
            currentValue >>= 6;
        } else {
            //bottom
            currentValue >>= 2;
        }
    } else {
        // Odd/right
        if (((i >> 4) & 0x01) == reverseAttributes) {
            // Top
            currentValue >>= 4;
        } else {
            // Bottom 
            currentValue >>= 0;
        }
    }
    assetTable[j] += currentValue;
}


// We need to reuse some variables here to save on memory usage. So, use #define to give them readable names.
// Note that this is ONLY a rename; if something relies on the original variable, that impacts this one too.
#define currentMemoryLocation tempInt2
// NOTE: tempChar1-tempChar3 are in use by update_player_sprite, which we call here. (Confusing, I know...)
#define bufferIndex tempChar8 
#define otherLoopIndex tempChar9


// reverseAttributes: If set to 1, this will flip which bits are used for the top and the bottom palette in the attribute table.
//                    This allows us to correctly draw starting on an odd-numbered row (such as at the start of our HUD.) 
void draw_current_map_to_nametable(int nametableAdr, int attributeTableAdr, unsigned char reverseAttributes) {

    // Prepare to draw on the first nametable
    set_vram_update(NULL);
    bufferIndex = 0;

    if (!reverseAttributes) {
        j = -1;
    } else {
        j = 7;
    }
    tempArrayIndex = NAMETABLE_UPDATE_PREFIX_LENGTH;
    for (i = 0; i != 192; ++i) {
         // The top 2 bytes of map data are palette data. Skip that for now.
        currentValue = currentMap[i] & 0x3f;
        // This bumps the tile id up from the id for a 16x16 tile to an 8x8 tile on the real map
        currentValue = (((currentValue & 0xf8)) << 2) + ((currentValue & 0x07) << 1);

        if (bufferIndex == 0) {
            currentMemoryLocation = nametableAdr +  ((i & 0xf0) << 2) + ((i % 16) << 1);
        }

        mapScreenBuffer[tempArrayIndex] = currentValue;
        mapScreenBuffer[tempArrayIndex + 1] = currentValue + 1;
        mapScreenBuffer[tempArrayIndex + 32] = currentValue + 16;
        mapScreenBuffer[tempArrayIndex + 33] = currentValue + 17;

        // okay, now we have to update the byte for palettes. This is going to look a bit messy...
        // Start with the top 2 bytes
        currentValue = currentMap[i] & 0xc0;

        // Update where we are going to update with the palette data, which we store in the buffer.
        if (reverseAttributes) {
            if ((i & 0x1f) == 0) {
                j -= 8;
            }
        } else {
            if ((i & 0x1f) == 16) {
                j -= 8;
            }
        }
        if ((i & 0x01) == 0) 
            j++;


        // Now based on where we are in the map, shift them appropriately.
        // This builds up the palette bytes - which comprise of 2 bits per 16x16 tile. It's a bit confusing...
        update_asset_table_based_on_current_value(reverseAttributes);

        // Every 16 frames, write the buffered data to the screen and start anew.
        ++bufferIndex;
        tempArrayIndex += 2;
        if (bufferIndex == 16) {
            bufferIndex = 0;
            tempArrayIndex = NAMETABLE_UPDATE_PREFIX_LENGTH;
            // Bunch of messy-looking stuff that tells neslib where to write this to the nametable, and how.
            mapScreenBuffer[0] = MSB(currentMemoryLocation) | NT_UPD_HORZ;
            mapScreenBuffer[1] = LSB(currentMemoryLocation);
            mapScreenBuffer[2] = 64;
            mapScreenBuffer[64 + NAMETABLE_UPDATE_PREFIX_LENGTH] = NT_UPD_EOF;
            set_vram_update(mapScreenBuffer);
            ppu_wait_nmi();
            set_vram_update(NULL);
            if (xScrollPosition != -1) {
                split_y(xScrollPosition, yScrollPosition);
            }

        }
    }
    // Draw the palette that we built up above.
    // Start by copying it into mapScreenBuffer, so we can tell neslib where this lives.
    for (i = 0; i != 0x38; ++i) {
        mapScreenBuffer[NAMETABLE_UPDATE_PREFIX_LENGTH + i] = assetTable[i];
    }
    mapScreenBuffer[0] = MSB(attributeTableAdr) | NT_UPD_HORZ;
    mapScreenBuffer[1] = LSB(attributeTableAdr);
    mapScreenBuffer[2] = 0x38;
    mapScreenBuffer[0x3b] = NT_UPD_EOF;
    set_vram_update(mapScreenBuffer);
    ppu_wait_nmi();
    set_vram_update(NULL);
    if (xScrollPosition != -1) {
        split_y(xScrollPosition, yScrollPosition);
    }

    
}

// Draw a row (technically two rows) of tiles onto the map. Breaks things up so we can hide
// the change behind the HUD while continuing to use vertical mirroring.
// This basically is the draw_current_map_to_nametable logic, but it stops after 32. 
// NOTE: i and j MUST be maintained between calls to this method.
void draw_individual_row(int nametableAdr, int attributeTableAdr, char oliChange) {
    while(1) {
         // The top 2 bytes of map data are palette data. Skip that for now.
        currentValue = currentMap[i] & 0x3f;
        // This bumps the tile id up from the id for a 16x16 tile to an 8x8 tile on the real map
        currentValue = (((currentValue >> 3)) << 5) + ((currentValue % 8) << 1);

        if (bufferIndex == 0) {
            currentMemoryLocation = nametableAdr +  ((i / 16) << 6) + ((i % 16) << 1);
        }

        // Figure out where to update the map, then store it so we don't keep calculating it.
        tempArrayIndex = NAMETABLE_UPDATE_PREFIX_LENGTH + (bufferIndex<<1);

        // Draw it to the map
        mapScreenBuffer[tempArrayIndex] = currentValue;
        mapScreenBuffer[tempArrayIndex + 1] = currentValue + 1;
        mapScreenBuffer[tempArrayIndex + 32] = currentValue + 16;
        mapScreenBuffer[tempArrayIndex + 33] = currentValue + 17;


        // okay, now we have to update the byte for palettes. This is going to look a bit messy...
        // Start with the top 2 bits
        currentValue = currentMap[i] & 0xc0;

        // Update where we are going to update with the palette data, which we store in the buffer.
        if (i % 32 == 16) 
            j -= 8;
        if ((i & 0x01) == 0) 
            j++;

        // Now based on where we are in the map, shift them appropriately.
        // This builds up the palette bytes - which comprise of 2 bits per 16x16 tile. It's a bit confusing...
        update_asset_table_based_on_current_value(0);

        // Every 16 frames, write the buffered data to the screen and start anew.
        ++bufferIndex;
        if (bufferIndex == 8) {
            ppu_wait_nmi();
            if (xScrollPosition != -1) {
                otherLoopIndex += oliChange;
                scroll(0, 240 - HUD_PIXEL_HEIGHT);
                split_y(256, 240 + 48 + otherLoopIndex);
            }
        }
        if (bufferIndex == 16) {
            bufferIndex = 0;
            // Bunch of messy-looking stuff that tells neslib where to write this to the nametable, and how.
            mapScreenBuffer[0] = MSB(currentMemoryLocation) | NT_UPD_HORZ;
            mapScreenBuffer[1] = LSB(currentMemoryLocation);
            mapScreenBuffer[2] = 64;
            // We wrote the 64 tiles in the loop above; they're ready to go.

            // Add in another update for the palette
            tempArrayIndex = 64 + NAMETABLE_UPDATE_PREFIX_LENGTH;
            load_palette_to_map_screen_buffer(attributeTableAdr);

            set_vram_update(mapScreenBuffer);
            ppu_wait_nmi();
            set_vram_update(NULL);
            if (xScrollPosition != -1) {
                scroll(0, 240 - HUD_PIXEL_HEIGHT);
                split_y(256, 240 + 48 + otherLoopIndex);
            }

        }
        ++i;
        if (i % 32 == 0) {
            break;
        }
    }

}

// The same method as above, but offset slightly on y to allow for smooth scrolling up.
void draw_individual_row_offset_y(int nametableAdr, int attributeTableAdr, char oliChange) {
    while(1) {
         // The top 2 bytes of map data are palette data. Skip that for now.
        currentValue = currentMap[i] & 0x3f;
        // This bumps the tile id up from the id for a 16x16 tile to an 8x8 tile on the real map
        currentValue = (((currentValue >> 3)) << 5) + ((currentValue % 8) << 1);

        if (bufferIndex == 0) {
            currentMemoryLocation = nametableAdr +  ((i / 16) << 6) + ((i % 16) << 1);
        }

        // Figure out where to update the map, then store it so we don't keep calculating it.
        tempArrayIndex = NAMETABLE_UPDATE_PREFIX_LENGTH + (bufferIndex<<1);

        // Draw it to the map
        mapScreenBuffer[tempArrayIndex] = currentValue;
        mapScreenBuffer[tempArrayIndex + 1] = currentValue + 1;
        mapScreenBuffer[tempArrayIndex + 32] = currentValue + 16;
        mapScreenBuffer[tempArrayIndex + 33] = currentValue + 17;


        // okay, now we have to update the byte for palettes. This is going to look a bit messy...
        // Start with the top 2 bits
        currentValue = currentMap[i] & 0xc0;

        // Update where we are going to update with the palette data, which we store in the buffer.
        if (i % 32 == 0) 
            j -= 8;
        if ((i & 0x01) == 0) 
            j++;

        // Now based on where we are in the map, shift them appropriately.
        // This builds up the palette bytes - which comprise of 2 bits per 16x16 tile. It's a bit confusing...
        update_asset_table_based_on_current_value(1);

        // Every 16 frames, write the buffered data to the screen and start anew.
        ++bufferIndex;
        if (bufferIndex == 8) {
            ppu_wait_nmi();
            if (xScrollPosition != -1) {
                otherLoopIndex += oliChange;
                scroll(0, 240 - HUD_PIXEL_HEIGHT);
                split_y(256, 240 - otherLoopIndex);
            }
        }
        if (bufferIndex == 16) {
            bufferIndex = 0;
            // Bunch of messy-looking stuff that tells neslib where to write this to the nametable, and how.
            mapScreenBuffer[0] = MSB(currentMemoryLocation) | NT_UPD_HORZ;
            mapScreenBuffer[1] = LSB(currentMemoryLocation);
            mapScreenBuffer[2] = 64;
            // We wrote the 64 tiles in the loop above; they're ready to go.
            
            mapScreenBuffer[63 + NAMETABLE_UPDATE_PREFIX_LENGTH + 1] = NT_UPD_EOF;
            set_vram_update(mapScreenBuffer);
            ppu_wait_nmi();
            set_vram_update(NULL);
            if (xScrollPosition != -1) {
                scroll(0, 240 - HUD_PIXEL_HEIGHT);
                split_y(256, 240 - otherLoopIndex);
            }
        }
        ++i;
        if (i % 32 == 0) {
            // Add in another update for the palette
            tempArrayIndex = 0;
            load_palette_to_map_screen_buffer(attributeTableAdr);

            set_vram_update(mapScreenBuffer);
            ppu_wait_nmi();
            set_vram_update(NULL);
            if (xScrollPosition != -1) {
                scroll(0, 240 - HUD_PIXEL_HEIGHT);
                split_y(256, 240 - otherLoopIndex);
            }

            break;
        }
    }

}

void draw_current_row_palette_only(int attributeTableAdr) {
    while(1) {

        // Get just the palette bits from this map tile
        currentValue = currentMap[i] & 0xc0;

        // Update where we are going to update with the palette data, which we store in the buffer.
        if (i % 32 == 0) 
            j -= 8;
        if ((i & 0x01) == 0) 
            j++;

        // Now based on where we are in the map, shift them appropriately.
        // This builds up the palette bytes - which comprise of 2 bits per 16x16 tile. It's a bit confusing...
        // Now based on where we are in the map, shift them appropriately.
        // This builds up the palette bytes - which comprise of 2 bits per 16x16 tile. It's a bit confusing...
        update_asset_table_based_on_current_value(1);

        // Every 16 frames, write the buffered data to the screen and start anew.
        ++bufferIndex;
        
        ++i;
        if (i % 32 == 0) {
            // Add in another update for the palette
            tempArrayIndex = 0;
            load_palette_to_map_screen_buffer(attributeTableAdr);

            set_vram_update(mapScreenBuffer);
            ppu_wait_nmi();
            set_vram_update(NULL);
            if (xScrollPosition != -1) {
                scroll(0, 240 - HUD_PIXEL_HEIGHT);
                split_y(256, 240 - otherLoopIndex);
            }

            break;
        }
    }

}

void draw_current_map_to_a(void) {
    ppu_off();

    /* Fill visible tiles with the actual blank maze tile, then reset all attributes. */
    vram_adr(0x2000);
    vram_fill(0x00, 0x03c0);
    vram_adr(0x23c0);
    for (i = 0; i < sizeof(pacman_maze_attr_bytes); ++i) {
        vram_put(pacman_maze_attr_bytes[i]);
    }

    /* Draw the original maze tile stream at the same PPU position as the ROM uploader ($2040). */
    for (i = 0; i < MAZE_ROWS; ++i) {
        vram_adr(NTADR_A(MAZE_COL_OFFSET, i + MAZE_ROW_OFFSET));
        for (j = 0; j < MAZE_COLS; ++j) {
            vram_put(maze_get_bg_tile(i, j));
        }
    }
    ppu_on_all();
}

void draw_current_map_to_b(void) {
    clear_asset_table(0);
    xScrollPosition = -1;
    yScrollPosition = 0;
    draw_current_map_to_nametable(NAMETABLE_B, NAMETABLE_B_ATTRS, 0);
}

void draw_current_map_to_c(void) {
    clear_asset_table(0);
    xScrollPosition = -1;
    yScrollPosition = 0;
    draw_current_map_to_nametable(NAMETABLE_C, NAMETABLE_C_ATTRS, 0);
}

void draw_current_map_to_d(void) {
    clear_asset_table(0);
    xScrollPosition = -1;
    yScrollPosition = 0;
    draw_current_map_to_nametable(NAMETABLE_D, NAMETABLE_D_ATTRS, 0);
}

// A quick, low-tech glamour-free way to transition between screens.
void do_fade_screen_transition(void) {
    load_map();
    load_sprites();
    clear_asset_table(1);
    fade_out_fast();
    
    // Now that the screen is clear, migrate the player's sprite a bit..
    if (playerDirection == SPRITE_DIRECTION_LEFT) {
        playerXPosition = (SCREEN_EDGE_RIGHT << PLAYER_POSITION_SHIFT);
    } else if (playerDirection == SPRITE_DIRECTION_RIGHT) {
        playerXPosition = (SCREEN_EDGE_LEFT << PLAYER_POSITION_SHIFT);
    } else if (playerDirection == SPRITE_DIRECTION_UP) {
        playerYPosition = (SCREEN_EDGE_BOTTOM << PLAYER_POSITION_SHIFT);
    } else if (playerDirection == SPRITE_DIRECTION_DOWN) {
        playerYPosition = (SCREEN_EDGE_TOP << PLAYER_POSITION_SHIFT);
    }
    // Actually move the sprite too, since otherwise this won't happen until after we un-blank the screen.
    banked_call(PRG_BANK_PLAYER_SPRITE, update_player_sprite);

    // Draw the updated map to the screen...
    draw_current_map_to_nametable(NAMETABLE_A, NAMETABLE_A_ATTRS, 0);
    
    // Update sprites once to make sure we don't show a flash of the old sprite positions.
    banked_call(PRG_BANK_MAP_SPRITES, update_map_sprites);
    fade_in_fast();
    // Aand we're back!
    gameState = GAME_STATE_RUNNING;
}

// Use a scrolling animation to move the player to the next screen.
void do_scroll_screen_transition(void) {
    // First, draw the next tile onto b
    xScrollPosition = -1;
    yScrollPosition = 0;
    scroll(0, 240 - HUD_PIXEL_HEIGHT);
    
    // Draw a sprite into 0 to give us something to split on
    oam_spr(249, HUD_PIXEL_HEIGHT-NES_SPRITE_HEIGHT-0, HUD_SPRITE_ZERO_TILE_ID, 0x00, 0);
    set_vram_update(NULL);
    ppu_wait_nmi();

    if (playerDirection == SPRITE_DIRECTION_RIGHT) {
        load_map();

        clear_asset_table(1);
        draw_current_map_to_nametable(NAMETABLE_B, NAMETABLE_B_ATTRS, 0);
        for (i = 0; i != 254; i+= SCREEN_SCROLL_LOOP_INCREMENT_LR) {
            playerXPosition -= SCREEN_SCROLL_MOVEMENT_INCREMENT_LR;
            banked_call(PRG_BANK_PLAYER_SPRITE, update_player_sprite);
            if (i % SCREEN_SCROLL_SPEED == 0) {
                ppu_wait_nmi();
                split(i, 0);
            }
        }
        xScrollPosition = 256;
        // Now, draw back to our original nametable...
        clear_asset_table(1);
        load_sprites();
        draw_current_map_to_nametable(NAMETABLE_A, NAMETABLE_A_ATTRS, 0);

    } else if (playerDirection == SPRITE_DIRECTION_LEFT) {
        load_map();

        clear_asset_table(1);
        draw_current_map_to_nametable(NAMETABLE_B, NAMETABLE_B_ATTRS, 0);
        for (i = 0; i != 254; i+= SCREEN_SCROLL_LOOP_INCREMENT_LR) {
            playerXPosition += SCREEN_SCROLL_MOVEMENT_INCREMENT_LR;
            banked_call(PRG_BANK_PLAYER_SPRITE, update_player_sprite);
            if (i % SCREEN_SCROLL_SPEED == 0) {
                ppu_wait_nmi();
                split(512-i, 0);
            }
        }
        xScrollPosition = 256;
        // Now, draw back to our original nametable...
        clear_asset_table(1);
        load_sprites();
        draw_current_map_to_nametable(NAMETABLE_A, NAMETABLE_A_ATTRS, 0);

    } else if (playerDirection == SPRITE_DIRECTION_DOWN) {
        // First draw original map to the other nametable
        clear_asset_table(0);
        draw_current_map_to_nametable(NAMETABLE_B + (SCREEN_WIDTH_TILES*6), NAMETABLE_B_ATTRS + 8, 1);
        
        load_map();
        // Loop over the screen, drawing the map in the space taken up by the hud every time we go 32 lines (2 tiles)
        // NOTE: We use both i and j in the loop inside one of the functions we're calling, so we needed another variable.
        clear_asset_table(0);
        i = 0; 
        j = -1;
        xScrollPosition = 256;
        yScrollPosition = 0;
        for (otherLoopIndex = 0; otherLoopIndex < 240 - HUD_PIXEL_HEIGHT; otherLoopIndex += SCREEN_SCROLL_LOOP_INCREMENT_UD) {

            playerYPosition -= SCREEN_SCROLL_MOVEMENT_INCREMENT_UD;
            banked_call(PRG_BANK_PLAYER_SPRITE, update_player_sprite);
            if (otherLoopIndex % 32 == 0 && otherLoopIndex < 224) {
                ppu_wait_nmi();
                split_y(256, 240 + HUD_PIXEL_HEIGHT + otherLoopIndex);

                draw_individual_row(NAMETABLE_B, NAMETABLE_B_ATTRS, SCREEN_SCROLL_LOOP_INCREMENT_UD);
            } else {
                if ((i % (SCREEN_SCROLL_SPEED*4)) == 0) {
                    ppu_wait_nmi();
                    split_y(256, 240 + HUD_PIXEL_HEIGHT + otherLoopIndex);
                }
            }
        }

        xScrollPosition = 256;
        // Bump otherLoopIndex back to where it was last animation frame; we don't want to kee updating.
        otherLoopIndex -= 2;
        // Now, draw back to our original nametable...
        clear_asset_table(1);
        load_sprites();
        ppu_wait_nmi();
        split_y(256, 240 + HUD_PIXEL_HEIGHT + otherLoopIndex);
        draw_current_map_to_nametable(NAMETABLE_A, NAMETABLE_A_ATTRS, 0);

    } else if (playerDirection == SPRITE_DIRECTION_UP) {
        // First draw original map to the other nametable
        clear_asset_table_skip_top();

        set_vram_update(NULL);
        bufferIndex = 0;

        load_map();
        xScrollPosition = 0;
        yScrollPosition = 0;
                
        // Draw the first line outside the general loop while this line is offscreen.
        i = 240 - (48 + 32);
        j = (i >> 2) + 7;
        otherLoopIndex = 0;
        draw_individual_row_offset_y(NAMETABLE_B + (SCREEN_WIDTH_TILES*6), NAMETABLE_B_ATTRS + 8, 0);


        // Loop over the screen, drawing the map in the space taken up by the hud every time we go 32 lines (2 tiles)
        // NOTE: We use both i and j in the loop inside one of the functions we're calling, so we needed another variable.
        i = 0; 
        j = -1;
        for (i = sizeof(assetTable) - 16; i < sizeof(assetTable) - 8 ; ++i) {
            assetTable[i] = assetTable[i] & 0xf0;
        }

        for (otherLoopIndex = 0; otherLoopIndex != 240 - HUD_PIXEL_HEIGHT; otherLoopIndex += SCREEN_SCROLL_LOOP_INCREMENT_UD) {
            playerYPosition += SCREEN_SCROLL_MOVEMENT_INCREMENT_UD;
            banked_call(PRG_BANK_PLAYER_SPRITE, update_player_sprite);
            if (otherLoopIndex % 32 == 0) {

                ppu_wait_nmi();
                split_y(256, 240 - (otherLoopIndex));

                // The 64 here is to hide this behind the hud, since we are drawing while still doing vertical mirroring.
                i = 240 - (HUD_PIXEL_HEIGHT + 64 + otherLoopIndex);
                // Special case for the asset table - we wrote to this part of it above already; this prevents glitching by adding it twice.
                if (i == 0) {
                    for (j = 0; j < 8; ++j) {
                        assetTable[j] = assetTable[j] & 0xf0;
                    }

                }
                j = (i >> 2) + 7;
                draw_individual_row_offset_y(NAMETABLE_B + (SCREEN_WIDTH_TILES*6), NAMETABLE_B_ATTRS + 8, SCREEN_SCROLL_LOOP_INCREMENT_UD);

                // Draw the palette for row 0 separately - have to do it here after we've loaded all of the assetTable stuff before.
                if (i == 0) {
                    j = (i >> 2) - 1;
                    draw_current_row_palette_only(NAMETABLE_B_ATTRS + 8);
                }
            } else {
                if (i % (SCREEN_SCROLL_SPEED<<1) == 0) {
                    ppu_wait_nmi();
                    split_y(256, 240 - (otherLoopIndex));
                }
            }
        }

        xScrollPosition = 256;
        load_sprites();

        // Now, draw back to our original nametable...
        ppu_wait_nmi();
        split_y(256, 240 - (otherLoopIndex));
        clear_asset_table(1);

        yScrollPosition = 240 - otherLoopIndex;
        draw_current_map_to_nametable(NAMETABLE_A, NAMETABLE_A_ATTRS, 0);
        

        // and bump the player back to the first screen now that we're done.
        scroll(0, 240 - HUD_PIXEL_HEIGHT);
        xScrollPosition = 0;
        yScrollPosition = 0;

        // Redraw to B to work around a bug that manifests itself if we scroll
        // up a second time, since we expect this to have been drawn to B in its normal location.
        clear_asset_table(1);
        draw_current_map_to_nametable(NAMETABLE_B, NAMETABLE_B_ATTRS, 0);

    }

    // and bump the player back to the first screen now that we're done.
    scroll(0, 240 - HUD_PIXEL_HEIGHT);

    // Hide sprite 0 - it has now served its purpose.
    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, HUD_SPRITE_ZERO_TILE_ID, 0x00, 0);

    xScrollPosition = -1;
    gameState = GAME_STATE_RUNNING;

}
