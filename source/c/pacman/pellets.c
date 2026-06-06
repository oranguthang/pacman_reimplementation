#include "source/c/pacman/pellets.h"

#include "source/c/map/map.h"
#include "source/c/configuration/system_constants.h"
#include "source/c/neslib.h"
#include "source/c/globals.h"
#include "source/c/graphics/hud.h"
#include "source/c/pacman/ghosts.h"
#include "source/c/pacman/score.h"
#include "source/c/sprites/player.h"

CODE_BANK(PRG_BANK_MAP_LOGIC);

#define PACMAN_MAZE_ROWS 27
#define PACMAN_MAZE_COLS 22
#define PACMAN_MAZE_ROW_OFFSET 2
#define PACMAN_MAZE_COL_OFFSET 0

#define PACMAN_PELLET_STATE_BYTES 75
#define PACMAN_PELLET_CLEAR_TILE 0x00
#define PACMAN_PELLET_TOTAL 192
#define PACMAN_PELLET_POINTS 10
#define PACMAN_POWER_PELLET_POINTS 50

static const unsigned char k_initial_pellet_state[PACMAN_PELLET_STATE_BYTES] = {
    0x00, 0x00, 0x00, 0xFF, 0xFD, 0x47, 0x44, 0x11,
    0x11, 0x51, 0x44, 0x44, 0x14, 0x11, 0xFF, 0xFF,
    0x47, 0x14, 0x14, 0x11, 0x05, 0x45, 0x7C, 0x77,
    0x1F, 0x10, 0x40, 0x00, 0x04, 0x10, 0x00, 0x01,
    0x04, 0x40, 0x00, 0x01, 0x10, 0x40, 0x00, 0x04,
    0x10, 0x00, 0x01, 0x04, 0x40, 0x00, 0x01, 0x10,
    0x40, 0xC0, 0x7F, 0xFF, 0x11, 0x51, 0x44, 0xDC,
    0xF7, 0x1D, 0x54, 0x50, 0x01, 0x15, 0x54, 0xF0,
    0xDD, 0x7D, 0x04, 0x14, 0x10, 0xFF, 0xFF, 0x07,
    0x00, 0x00, 0x00
};

unsigned char pacmanPelletsRemaining;

static unsigned char s_pellet_state[PACMAN_PELLET_STATE_BYTES];
static unsigned char s_pellet_update_active;

static unsigned int pellet_bit_index(unsigned char row, unsigned char col) {
    return ((unsigned int)row * PACMAN_MAZE_COLS) + col;
}

static unsigned char pellet_bit_mask(unsigned int index) {
    return 1 << (index & 0x07);
}

static unsigned char pellet_state_get(const unsigned char* state, unsigned char row, unsigned char col) {
    unsigned int index = pellet_bit_index(row, col);
    return state[index >> 3] & pellet_bit_mask(index);
}

static unsigned char pellet_is_active(unsigned char row, unsigned char col) {
    return pellet_state_get(s_pellet_state, row, col);
}

static unsigned char pellet_type_at(unsigned char row, unsigned char col) {
    if ((row == 3 || row == 20) && (col == 2 || col == 20)) {
        return PACMAN_PELLET_POWER;
    }

    return PACMAN_PELLET_REGULAR;
}

static void pellet_clear(unsigned char row, unsigned char col) {
    unsigned int index = pellet_bit_index(row, col);
    s_pellet_state[index >> 3] &= (unsigned char)~pellet_bit_mask(index);
}

static void append_score_digits_to_buffer(const unsigned char* digits) {
    unsigned char index;
    unsigned char saw_non_zero = 0;

    for (index = 0; index < PACMAN_SCORE_DIGIT_COUNT; ++index) {
        if (digits[index] != 0 || index >= (PACMAN_SCORE_DIGIT_COUNT - 2)) {
            saw_non_zero = 1;
        }

        if (saw_non_zero) {
            screenBuffer[i++] = '0' + digits[index];
        } else {
            screenBuffer[i++] = HUD_TILE_BLANK;
        }
    }
}

static void queue_pellet_score_updates(unsigned char tile_col, unsigned char tile_row) {
    i = 0;

    screenBuffer[i++] = MSB(NTADR_A(tile_col, tile_row));
    screenBuffer[i++] = LSB(NTADR_A(tile_col, tile_row));
    screenBuffer[i++] = PACMAN_PELLET_CLEAR_TILE;

    if (pacmanScoreDirty) {
        screenBuffer[i++] = MSB(HUD_SCORE_ADDR) | NT_UPD_HORZ;
        screenBuffer[i++] = LSB(HUD_SCORE_ADDR);
        screenBuffer[i++] = PACMAN_SCORE_DIGIT_COUNT;
        append_score_digits_to_buffer(pacmanScoreDigits);
    }

    if (pacmanHiScoreDirty) {
        screenBuffer[i++] = MSB(HUD_HI_SCORE_ADDR) | NT_UPD_HORZ;
        screenBuffer[i++] = LSB(HUD_HI_SCORE_ADDR);
        screenBuffer[i++] = PACMAN_SCORE_DIGIT_COUNT;
        append_score_digits_to_buffer(pacmanHiScoreDigits);
    }

    screenBuffer[i++] = NT_UPD_EOF;
    set_vram_update(screenBuffer);
    s_pellet_update_active = 1;
    pacmanScoreDirty = 0;
    pacmanHiScoreDirty = 0;
}

void pacman_pellets_init(void) {
    unsigned char index;

    for (index = 0; index < PACMAN_PELLET_STATE_BYTES; ++index) {
        s_pellet_state[index] = k_initial_pellet_state[index];
    }

    pacmanPelletsRemaining = PACMAN_PELLET_TOTAL;
    s_pellet_update_active = 0;
    set_vram_update(NULL);
}

void pacman_pellets_poll(void) {
    if (s_pellet_update_active) {
        set_vram_update(NULL);
        s_pellet_update_active = 0;
    }
}

unsigned char pacman_pellets_adjust_maze_tile(unsigned char row, unsigned char col, unsigned char tile_id) {
    if (tile_id != 0x03 && tile_id != 0x09 &&
        tile_id != 0x01 && tile_id != 0x02) {
        return tile_id;
    }

    if (!pellet_is_active(row, col)) {
        return PACMAN_PELLET_CLEAR_TILE;
    }

    return tile_id;
}

void pacman_pellets_try_eat(void) {
    unsigned char pixel_x = (unsigned char)((playerXPosition >> PLAYER_POSITION_SHIFT) + NES_SPRITE_WIDTH);
    unsigned char pixel_y = (unsigned char)((playerYPosition >> PLAYER_POSITION_SHIFT) + NES_SPRITE_HEIGHT);
    unsigned char tile_col = pixel_x >> 3;
    unsigned char tile_row = pixel_y >> 3;
    unsigned char maze_row;
    unsigned char maze_col;

    if (tile_row < PACMAN_MAZE_ROW_OFFSET || tile_row >= (PACMAN_MAZE_ROW_OFFSET + PACMAN_MAZE_ROWS)) {
        return;
    }
    if (tile_col >= (PACMAN_MAZE_COL_OFFSET + PACMAN_MAZE_COLS)) {
        return;
    }

    maze_row = tile_row - PACMAN_MAZE_ROW_OFFSET;
    maze_col = tile_col - PACMAN_MAZE_COL_OFFSET;
    if (!pellet_is_active(maze_row, maze_col)) {
        return;
    }

    pellet_clear(maze_row, maze_col);
    if (pacmanPelletsRemaining != 0) {
        --pacmanPelletsRemaining;
    }

    if (pellet_type_at(maze_row, maze_col) == PACMAN_PELLET_POWER) {
        pacman_ghosts_start_frightened();
        pacman_score_add(PACMAN_POWER_PELLET_POINTS);
    } else {
        pacman_score_add(PACMAN_PELLET_POINTS);
    }

    queue_pellet_score_updates(tile_col, tile_row);
}
