/*
 * title.c - Pac-Man title screen wrapper
 * Uses the more accurate renderer/state machine from source/c/pacman/.
 */

#include "source/c/neslib.h"
#include "source/c/menus/title.h"
#include "source/c/globals.h"
#include "source/c/configuration/game_states.h"
#include "source/c/configuration/system_constants.h"
#include "source/c/library/bank_helpers.h"
#include "source/c/pacman/demo.h"
#include "source/c/pacman/title.h"

CODE_BANK(PRG_BANK_TITLE);

void draw_title_screen(void) {
    set_chr_bank_0(CHR_BANK_TILES);
    set_chr_bank_1(CHR_BANK_TILES);
    oam_clear();
    title_init();
    gameState = GAME_STATE_TITLE_INPUT;
}

void handle_title_input(void) {
    unsigned char players_2p;
    unsigned char result = title_update(&players_2p);

    if (result == 1) {
        set_vram_update(NULL);
        gameState = GAME_STATE_POST_TITLE;
    } else if (result == 2) {
        set_vram_update(NULL);
        music_stop();
        demo_init();
        gameState = GAME_STATE_DEMO;
    }
}
