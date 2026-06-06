#include "source/c/neslib.h"
#include "source/c/graphics/hud.h"
#include "source/c/configuration/system_constants.h"
#include "source/c/globals.h"
#include "source/c/pacman/score.h"

CODE_BANK(PRG_BANK_HUD);

#define HUD_PAUSE_ATTR_VALUE 0x55
#define HUD_LIVES_TO_DRAW    2

static const unsigned char k_hud_label_1up[] = {
    0xB0, 0xB3, 0xB2
};

static const unsigned char k_hud_label_hi_score[] = {
    0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB
};

static const unsigned char k_hud_label_pause[] = {
    'P', 'A', 'U', 'S', 'E'
};

static unsigned char s_hud_update_active;
static unsigned char s_hud_paused;
static unsigned char s_hud_pause_dirty;

#define HUD_ATTR_SCORE_ROW0_ADDR (NAMETABLE_A_ATTRS + 0x05)
#define HUD_ATTR_SCORE_ROW1_ADDR (NAMETABLE_A_ATTRS + 0x0D)
#define HUD_ATTR_SCORE_ROW2_ADDR (NAMETABLE_A_ATTRS + 0x15)
#define HUD_ATTR_PAUSE_ADDR      (NAMETABLE_A_ATTRS + 0x25)

static void write_abs(unsigned int addr, const unsigned char* data, unsigned char len) {
    unsigned char index;

    vram_adr(addr);
    for (index = 0; index < len; ++index) {
        vram_put(data[index]);
    }
}

static void write_score_digits_direct(unsigned int addr, const unsigned char* digits) {
    unsigned char index;
    unsigned char saw_non_zero = 0;

    vram_adr(addr);
    for (index = 0; index < PACMAN_SCORE_DIGIT_COUNT; ++index) {
        if (digits[index] != 0 || index >= (PACMAN_SCORE_DIGIT_COUNT - 2)) {
            saw_non_zero = 1;
        }

        if (saw_non_zero) {
            vram_put('0' + digits[index]);
        } else {
            vram_put(HUD_TILE_BLANK);
        }
    }
}

static void write_blank_run(unsigned int addr, unsigned char len) {
    vram_adr(addr);
    vram_fill(HUD_TILE_BLANK, len);
}

static void draw_life_icons(void) {
    unsigned char icon_index;
    unsigned int addr;

    for (icon_index = 0; icon_index < HUD_LIVES_TO_DRAW; ++icon_index) {
        addr = HUD_LIVES_ADDR + (icon_index << 1);
        vram_adr(addr);
        vram_put(0x3C);
        vram_put(0x3D);
        vram_adr(addr + 0x20);
        vram_put(0x3E);
        vram_put(0x3F);
    }
}

static void draw_pause_label_direct(void) {
    if (s_hud_paused) {
        write_abs(HUD_LABEL_PAUSE_ADDR, k_hud_label_pause, sizeof(k_hud_label_pause));
    } else {
        write_blank_run(HUD_LABEL_PAUSE_ADDR, sizeof(k_hud_label_pause));
    }
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

static void append_pause_label_to_buffer(void) {
    unsigned char index;

    for (index = 0; index < sizeof(k_hud_label_pause); ++index) {
        if (s_hud_paused) {
            screenBuffer[i++] = k_hud_label_pause[index];
        } else {
            screenBuffer[i++] = HUD_TILE_BLANK;
        }
    }
}

void draw_hud(void) {
    ppu_off();

    s_hud_paused = 0;
    s_hud_pause_dirty = 0;

    /* Keep maze quadrants untouched and recolor only the HUD quadrants on the right. */
    vram_adr(HUD_ATTR_SCORE_ROW0_ADDR);
    vram_put(0x91);
    vram_put(0xA0);
    vram_put(0x20);
    vram_adr(HUD_ATTR_SCORE_ROW1_ADDR);
    vram_put(0x91);
    vram_put(0xA0);
    vram_put(0x00);
    vram_adr(HUD_ATTR_SCORE_ROW2_ADDR);
    vram_put(0x11);
    vram_put(0x00);
    vram_put(0x00);

    vram_adr(HUD_ATTR_PAUSE_ADDR);
    vram_put(HUD_PAUSE_ATTR_VALUE);
    vram_put(HUD_PAUSE_ATTR_VALUE);

    write_blank_run(HUD_LABEL_HI_SCORE_ADDR, sizeof(k_hud_label_hi_score));
    write_blank_run(HUD_HI_SCORE_ADDR, PACMAN_SCORE_DIGIT_COUNT);
    write_blank_run(HUD_LABEL_1UP_ADDR, sizeof(k_hud_label_1up));
    write_blank_run(HUD_SCORE_ADDR, PACMAN_SCORE_DIGIT_COUNT);
    write_blank_run(HUD_LABEL_PAUSE_ADDR, sizeof(k_hud_label_pause));

    write_abs(HUD_LABEL_1UP_ADDR, k_hud_label_1up, sizeof(k_hud_label_1up));
    write_abs(HUD_LABEL_HI_SCORE_ADDR, k_hud_label_hi_score, sizeof(k_hud_label_hi_score));
    write_score_digits_direct(HUD_SCORE_ADDR, pacmanScoreDigits);
    write_score_digits_direct(HUD_HI_SCORE_ADDR, pacmanHiScoreDigits);
    draw_life_icons();
    draw_pause_label_direct();

    set_vram_update(NULL);
    s_hud_update_active = 0;
    pacmanScoreDirty = 0;
    pacmanHiScoreDirty = 0;
    ppu_on_all();
}

static void set_hud_paused_state(unsigned char is_paused) {
    if (s_hud_paused == is_paused) {
        return;
    }

    s_hud_paused = is_paused;
    s_hud_pause_dirty = 1;
}

void show_hud_pause(void) {
    set_hud_paused_state(1);
}

void hide_hud_pause(void) {
    set_hud_paused_state(0);
}

void update_hud(void) {
    if (s_hud_update_active) {
        set_vram_update(NULL);
        s_hud_update_active = 0;
    }

    if (!pacmanScoreDirty && !pacmanHiScoreDirty && !s_hud_pause_dirty) {
        return;
    }

    i = 0;
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

    if (s_hud_pause_dirty) {
        screenBuffer[i++] = MSB(HUD_LABEL_PAUSE_ADDR) | NT_UPD_HORZ;
        screenBuffer[i++] = LSB(HUD_LABEL_PAUSE_ADDR);
        screenBuffer[i++] = sizeof(k_hud_label_pause);
        append_pause_label_to_buffer();
    }

    screenBuffer[i++] = NT_UPD_EOF;
    set_vram_update(screenBuffer);
    s_hud_update_active = 1;
    pacmanScoreDirty = 0;
    pacmanHiScoreDirty = 0;
    s_hud_pause_dirty = 0;
}
