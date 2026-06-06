#include "source/c/pacman/compat.h"
#include "source/c/pacman/render.h"
#include "source/c/pacman/title.h"
#include "source/c/menus/title.h"
#include "source/c/library/bank_helpers.h"

CODE_BANK(PRG_BANK_TITLE);

#define TITLE_DEMO_TIMEOUT_FRAMES 0x01ECu

enum {
    TITLE_STATE_SLIDE = 0,
    TITLE_STATE_MENU  = 1
};

static unsigned char title_state;
static unsigned char title_scroll_y;
static unsigned int  title_menu_timer;
static unsigned char title_players_2p;
static unsigned char select_latch;
static unsigned char start_latch;

/*
 * VRAM update buffer for the cursor arrow (2 single-byte writes).
 * Format per neslib: MSB, LSB, byte  ...  NT_UPD_EOF
 * Cursor is at NTADR_A(10,16) = 0x220A  and  NTADR_A(10,18) = 0x224A
 */
static unsigned char s_cursor_buf[7];

static void refresh_cursor_buf(void) {
    unsigned char top = title_players_2p ? 0x20 : 0x5C;
    unsigned char bot = title_players_2p ? 0x5C : 0x20;
    s_cursor_buf[0] = 0x22; s_cursor_buf[1] = 0x0A; s_cursor_buf[2] = top;
    s_cursor_buf[3] = 0x22; s_cursor_buf[4] = 0x4A; s_cursor_buf[5] = bot;
    s_cursor_buf[6] = NT_UPD_EOF;
}

void title_init(void) {
    title_state      = TITLE_STATE_SLIDE;
    /* Reference title roll starts on the lower nametable and counts up to 0xEF. */
    title_scroll_y   = 0x01;
    title_menu_timer = 0;
    title_players_2p = 0;
    select_latch     = 0;
    start_latch      = 0;

    render_title_full(title_scroll_y, title_players_2p, 1);
    refresh_cursor_buf();
    set_vram_update(NULL);
}

/*
 * Call once per frame BEFORE ppu_wait_nmi().
 * Updates neslib scroll variables so the NMI applies the correct scroll.
 * Returns: 0 = still on title, 1 = start game, 2 = go to demo.
 */
unsigned char title_update(unsigned char* players_2p) {
    unsigned char pad = pad_poll(0);
    unsigned char start_now  = (unsigned char)((pad & PAD_START)  != 0);
    unsigned char select_now = (unsigned char)((pad & PAD_SELECT) != 0);
    unsigned char start_edge  = (unsigned char)(start_now  && !start_latch);
    unsigned char select_edge = (unsigned char)(select_now && !select_latch);

    *players_2p = title_players_2p;

    if (title_state == TITLE_STATE_SLIDE) {
        if (pad & (PAD_START | PAD_SELECT)) {
            /* Skip the roll, jump straight to menu. */
            title_scroll_y   = 0;
            title_menu_timer = 0;
            title_state      = TITLE_STATE_MENU;
            scroll(0, 0);
            set_vram_update(s_cursor_buf); /* start cursor updates */
        } else if (title_scroll_y < 0xEF) {
            ++title_scroll_y;
            /* Use neslib scroll: nametable $2800 (240+y) for the roll effect. */
            scroll(0, (unsigned int)(240u + title_scroll_y));
        } else {
            /* Roll complete. */
            title_scroll_y   = 0;
            title_state      = TITLE_STATE_MENU;
            title_menu_timer = 0;
            scroll(0, 0);
            set_vram_update(s_cursor_buf);
        }
    } else {
        ++title_menu_timer;

        if (title_menu_timer >= TITLE_DEMO_TIMEOUT_FRAMES) {
            title_menu_timer = 0;
            start_latch  = start_now;
            select_latch = select_now;
            return 2; /* demo timeout */
        }

        if (select_edge) {
            title_players_2p  ^= 1;
            *players_2p        = title_players_2p;
            title_menu_timer   = 1;
            refresh_cursor_buf();
        }

        if (start_edge) {
            *players_2p = title_players_2p;
            start_latch  = start_now;
            select_latch = select_now;
            return 1; /* start game */
        }
    }

    start_latch  = start_now;
    select_latch = select_now;
    return 0;
}
