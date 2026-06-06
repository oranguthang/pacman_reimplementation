#include "source/c/pacman/compat.h"
#include "source/c/pacman/render.h"
#include "source/c/menus/title.h"
#include "source/c/configuration/system_constants.h"
#include "source/c/library/bank_helpers.h"

CODE_BANK(PRG_BANK_TITLE);

static const unsigned char k_title_bg_palette[16] = {
    0x0F, 0x20, 0x0F, 0x06, 0x0F, 0x26, 0x20, 0x27,
    0x0F, 0x06, 0x0F, 0x26, 0x0F, 0x06, 0x20, 0x26
};

static const unsigned char k_title_sprite_palette[16] = {
    0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F
};

static const unsigned char k_attract_palette_32[32] = {
    0x0F, 0x20, 0x0F, 0x06, 0x0F, 0x06, 0x0F, 0x33,
    0x0F, 0x33, 0x0F, 0x27, 0x0F, 0x17, 0x0F, 0x21,
    0x0F, 0x27, 0x20, 0x06, 0x0F, 0x11, 0x20, 0x33,
    0x0F, 0x20, 0x20, 0x21, 0x0F, 0x09, 0x20, 0x17
};

static const unsigned char k_title_hi_score[] = {
    0xB0, 0xB3, 0xB2, 0x20, 0x20, 0x20, 0x20, 0xB4, 0xB5, 0xB6, 0xB7,
    0xB8, 0xB9, 0xBA, 0xBB, 0x20, 0x20, 0x20, 0xB1, 0xB3, 0xB2
};

static const unsigned char k_title_attr_23c8[24] = {
    0x80, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0x00,
    0x00, 0x66, 0x55, 0x55, 0x55, 0x55, 0xDD, 0x00,
    0x08, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x00
};

static const unsigned char k_pacman_logo_tiles[144] = {
    0xE4, 0xE8, 0xE8, 0xE8, 0xE8, 0xE8, 0xE8, 0xE8, 0xE8, 0xE8, 0xE8, 0xE8,
    0xE8, 0xE8, 0xE8, 0xE8, 0xE8, 0xE8, 0xE8, 0xE8, 0xE8, 0xE8, 0xE5, 0xEB,
    0x88, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x88, 0x89,
    0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0xA3, 0xE9, 0xEB, 0x88,
    0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D,
    0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA3, 0xE9, 0xEB, 0x88, 0x92,
    0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xA3, 0xAF,
    0xD0, 0xD1, 0xD2, 0xA3, 0xD3, 0xA4, 0xA3, 0xE9, 0xEB, 0x88, 0xD4, 0xD5,
    0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0x88, 0x88, 0xDC, 0xD7, 0xDD, 0xDE,
    0xDF, 0xE0, 0xE1, 0xE2, 0xE3, 0xA3, 0xE9, 0xE7, 0xEA, 0xEA, 0xEA, 0xEA,
    0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
    0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xE6
};

static const unsigned char k_title_p1[]        = {0x5C, '-', '1', ' ', 'P', 'L', 'A', 'Y', 'E', 'R'};
static const unsigned char k_title_p2[]        = {'2', ' ', 'P', 'L', 'A', 'Y', 'E', 'R', 'S'};
static const unsigned char k_title_namcot[]    = {'#', '$', '%', '&', '\'', '(', ')', '*', '+'};
static const unsigned char k_title_copyright[] = {
    ']', ' ', '1', '9', '8', '0', ' ', '1', '9', '8', '4', ' ',
    'N', 'A', 'M', 'C', 'O', ' ', 'L', 'T', 'D', '['
};
static const unsigned char k_title_all_rights[] = {
    'A', 'L', 'L', ' ', 'R', 'I', 'G', 'H', 'T', 'S', ' ',
    'R', 'E', 'S', 'E', 'R', 'V', 'E', 'D'
};

static const unsigned char k_demo_pkt00_text[] = {
    'C', 'H', 'R', 'A', 'C', 'T', 'E', 'R', ' ', ' ', ';', ' ', ' ', 'N', 'I', 'C', 'K', 'N', 'A', 'M', 'E'
};
static const unsigned char k_demo_pkt02_text[] = {
    'O', 'I', 'K', 'A', 'K', 'E', '.', '.', '.', '.', '.'
};
static const unsigned char k_demo_pkt04_text[] = {'_', 'A', 'K', 'A', 'B', 'E', 'I', '_'};
static const unsigned char k_demo_pkt06_text[] = {'M', 'A', 'C', 'H', 'I', 'B', 'U', 'S', 'E', '.', '.'};
static const unsigned char k_demo_pkt08_text[] = {'_', 'P', 'I', 'N', 'K', 'Y', '_'};
static const unsigned char k_demo_pkt0A_text[] = {0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0x03, 0x03, 0x03};
static const unsigned char k_demo_pkt0C_text[] = {0xC8, 0xC3, 0xC9, 0xCA, 0xC5, 0xC0, 0xC7, 0xC8};
static const unsigned char k_demo_pkt0E_text[] = {'O', 'T', 'O', 'B', 'O', 'K', 'E', '.', '.', '.', '.'};
static const unsigned char k_demo_pkt10_text[] = {'_', 'G', 'U', 'Z', 'U', 'T', 'A', '_'};

static const unsigned char k_demo_sprite_strip_data[64] = {
    0x48, 0x1C, 0x40, 0x26, 0x48, 0x1B, 0x40, 0x2E, 0x50, 0x1F, 0x40, 0x26, 0x50, 0x1D, 0x40, 0x2E,
    0x60, 0x1C, 0x41, 0x26, 0x60, 0x1B, 0x41, 0x2E, 0x68, 0x1F, 0x41, 0x26, 0x68, 0x1D, 0x41, 0x2E,
    0x78, 0x1C, 0x42, 0x26, 0x78, 0x1B, 0x42, 0x2E, 0x80, 0x1F, 0x42, 0x26, 0x80, 0x1D, 0x42, 0x2E,
    0x90, 0x1C, 0x43, 0x26, 0x90, 0x1B, 0x43, 0x2E, 0x98, 0x1F, 0x43, 0x26, 0x98, 0x1D, 0x43, 0x2E
};

static void write_abs(unsigned int addr, const unsigned char* data, unsigned char len) {
    unsigned char i;
    vram_adr(addr);
    for (i = 0; i < len; ++i) {
        vram_put(data[i]);
    }
}

static void write_abs_direct_offset(unsigned int addr, unsigned int nt_offset, const unsigned char* data, unsigned char len) {
    write_abs((unsigned int)(addr + nt_offset), data, len);
}

static void upload_title_palette(void) {
    pal_bg(k_title_bg_palette);
    pal_spr(k_title_sprite_palette);
}

static void upload_attract_palette_32(void) {
    unsigned char i;
    vram_adr(0x3F00);
    for (i = 0; i < 32; ++i) {
        vram_put(k_attract_palette_32[i]);
    }
}

static void draw_title_attr(unsigned int addr) {
    unsigned char i;
    vram_adr(addr);
    for (i = 0; i < 8; ++i) {
        vram_put(0x00);
    }
    for (i = 0; i < 24; ++i) {
        vram_put(k_title_attr_23c8[i]);
    }
    for (i = 0; i < 32; ++i) {
        vram_put(0x00);
    }
}

static void draw_title_logo(unsigned int addr) {
    unsigned char row;
    unsigned char col;
    unsigned char idx = 0;

    for (row = 0; row < 6; ++row) {
        vram_adr(addr);
        for (col = 0; col < 23; ++col) {
            vram_put(k_pacman_logo_tiles[idx++]);
        }
        addr += 0x20;
    }
}

static void draw_score_row(unsigned int base) {
    static const unsigned char l[] = {'0', '0'};
    static const unsigned char h[] = {'1', '0', '0', '0', '0'};
    static const unsigned char r[] = {'0', '0'};

    write_abs(base + 0x0088, l, 2);
    write_abs(base + 0x008E, h, 5);
    write_abs(base + 0x009A, r, 2);
}

static void draw_title_page(unsigned int base, unsigned int attr) {
    draw_title_attr(attr);
    draw_title_logo(base + 0x00E5);
    write_abs(base + 0x0065, k_title_hi_score, sizeof(k_title_hi_score));
    write_abs(base + 0x020A, k_title_p1, sizeof(k_title_p1));
    write_abs(base + 0x024C, k_title_p2, sizeof(k_title_p2));
    write_abs(base + 0x02AC, k_title_namcot, sizeof(k_title_namcot));
    write_abs(base + 0x0305, k_title_copyright, sizeof(k_title_copyright));
    write_abs(base + 0x0347, k_title_all_rights, sizeof(k_title_all_rights));
    draw_score_row(base);
}

static void draw_demo_packet9_fast(void) {
    (void)PPU_STATUS_REG;

    PPU_ADDR_REG = 0x22;
    PPU_ADDR_REG = 0xAD;
    PPU_DATA_REG = 0x03;
    PPU_DATA_REG = 0x20;
    PPU_DATA_REG = '1';
    PPU_DATA_REG = '0';
    PPU_DATA_REG = 0x20;
    PPU_DATA_REG = 'P';
    PPU_DATA_REG = 'T';
    PPU_DATA_REG = 'S';

    PPU_ADDR_REG = 0x22;
    PPU_ADDR_REG = 0xED;
    PPU_DATA_REG = 0x01;
    PPU_DATA_REG = 0x20;
    PPU_DATA_REG = '5';
    PPU_DATA_REG = '0';
    PPU_DATA_REG = 0x20;
    PPU_DATA_REG = 'P';
    PPU_DATA_REG = 'T';
    PPU_DATA_REG = 'S';

    PPU_ADDR_REG = 0x23;
    PPU_ADDR_REG = 0x4C;
    PPU_DATA_REG = '#';
    PPU_DATA_REG = '$';
    PPU_DATA_REG = '%';
    PPU_DATA_REG = '&';
    PPU_DATA_REG = '\'';
    PPU_DATA_REG = '(';
    PPU_DATA_REG = ')';
    PPU_DATA_REG = '*';
    PPU_DATA_REG = '+';
}

static void oam_write_strip_at(unsigned char strip_idx, unsigned char oam_base, unsigned char* dst_buf) {
    unsigned char i;
    unsigned char base = (unsigned char)(strip_idx << 4);
    for (i = 0; i < 16; ++i) {
        dst_buf[(unsigned char)(oam_base + i)] = k_demo_sprite_strip_data[(unsigned char)(base + i)];
    }
}

static void draw_attract_playfield_frame(void) {
    unsigned char row;
    unsigned char col;

    vram_adr(0x20C0);
    for (row = 0; row < 24; ++row) {
        vram_put(0x2D);
        vram_put(0x2D);
        for (col = 0; col < 28; ++col) {
            vram_put(0x20);
        }
        vram_put(0x2D);
        vram_put(0x2D);
    }
}

static void draw_attract_attr_and_palette(void) {
    unsigned char i;
    unsigned char rep;
    unsigned char val;

    vram_adr(0x23C0);
    for (i = 0; i < 0x20; ++i) {
        vram_put(0x00);
    }

    vram_adr(0x23D0);
    val = 0x55;
    for (rep = 0; rep < 3; ++rep) {
        for (i = 0; i < 8; ++i) {
            vram_put(val);
        }
        val = (unsigned char)(val + 0x55);
    }

    upload_attract_palette_32();

    vram_adr(0x23E8);
    vram_put(0xAA);
    vram_put(0xAA);
    vram_put(0xAA);
    vram_put(0x22);
}

static void draw_demo_packet_text(unsigned char packet_idx) {
    switch (packet_idx) {
        case 0:
            write_abs(0x20E6, k_demo_pkt00_text, sizeof(k_demo_pkt00_text));
            break;
        case 1:
            write_abs(0x2148, k_demo_pkt02_text, sizeof(k_demo_pkt02_text));
            break;
        case 2:
            write_abs(0x2153, k_demo_pkt04_text, sizeof(k_demo_pkt04_text));
            break;
        case 3:
            write_abs(0x21A8, k_demo_pkt06_text, sizeof(k_demo_pkt06_text));
            break;
        case 4:
            write_abs(0x21B3, k_demo_pkt08_text, sizeof(k_demo_pkt08_text));
            break;
        case 5:
            write_abs(0x2208, k_demo_pkt0A_text, sizeof(k_demo_pkt0A_text));
            break;
        case 6:
            write_abs(0x2213, k_demo_pkt0C_text, sizeof(k_demo_pkt0C_text));
            break;
        case 7:
            write_abs(0x2268, k_demo_pkt0E_text, sizeof(k_demo_pkt0E_text));
            break;
        case 8:
            write_abs(0x2273, k_demo_pkt10_text, sizeof(k_demo_pkt10_text));
            break;
        case 9:
            draw_demo_packet9_fast();
            break;
        default:
            break;
    }
}

static void draw_demo_names_base(void) {
    static const unsigned char k_blank32[32] = {
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
    };
    static const unsigned char k_blank19[19] = {
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
    };

    ppu_clear_nt(0x2000, 0x20);
    ppu_clear_nt(0x2400, 0x20);
    ppu_clear_nt(0x2800, 0x20);
    ppu_clear_nt(0x2C00, 0x20);

    write_abs(0x2000, k_blank32, 32);
    write_abs(0x2020, k_blank32, 32);
    write_abs(0x2040, k_blank32, 32);
    write_abs(0x2347, k_blank19, 19);

    draw_attract_playfield_frame();
    draw_attract_attr_and_palette();
    write_abs_direct_offset(0x2065, 0x0000, k_title_hi_score, sizeof(k_title_hi_score));
    draw_score_row(0x2000);
}

void render_title_scroll(unsigned char scroll_y) {
    (void)PPU_STATUS_REG;
    PPU_CTRL_REG = 0x88;
    PPU_SCROLL_REG = 0;
    PPU_SCROLL_REG = scroll_y;
}

static void render_title_scroll_roll(unsigned char scroll_y) {
    (void)PPU_STATUS_REG;
    PPU_CTRL_REG = 0x8A;
    PPU_SCROLL_REG = 0;
    PPU_SCROLL_REG = scroll_y;
}

void render_demo_scroll(unsigned char scroll_y) {
    (void)PPU_STATUS_REG;
    PPU_CTRL_REG = 0x88;
    PPU_SCROLL_REG = 0;
    PPU_SCROLL_REG = scroll_y;
}

void render_title_cursor(unsigned char players_2p) {
    unsigned char top = players_2p ? 0x20 : 0x5C;
    unsigned char bot = players_2p ? 0x5C : 0x20;
    vram_adr(NTADR_A(10, 16));
    vram_put(top);
    vram_adr(NTADR_A(10, 18));
    vram_put(bot);
}

void render_title_full(unsigned char scroll_y, unsigned char players_2p, unsigned char slide_mode) {
    ppu_off();

    set_chr_bank_0(CHR_BANK_TILES);
    set_chr_bank_1(CHR_BANK_TILES);

    ppu_clear_nt(0x2000, 0x20);
    ppu_clear_nt(0x2400, 0x20);
    ppu_clear_nt(0x2800, 0x20);
    ppu_clear_nt(0x2C00, 0x20);

    upload_title_palette();
    draw_title_page(0x2000, 0x23C0);
    draw_title_page(0x2800, 0x2BC0);
    render_title_cursor(players_2p);

    if (slide_mode) {
        render_title_scroll_roll(scroll_y);
    } else {
        render_title_scroll(0);
    }

    ppu_on_all();
}

void render_demo_names_screen(void) {
    ppu_off();

    set_chr_bank_0(CHR_BANK_TILES);
    set_chr_bank_1(CHR_BANK_TILES);

    draw_demo_names_base();
    draw_demo_packet_text(0);
    oam_clear();
    render_demo_scroll(0);
    ppu_on_all();
}

void render_demo_names_packet_vblank(unsigned char packet_idx) {
    if (packet_idx >= 10) {
        return;
    }
    draw_demo_packet_text(packet_idx);
}

void render_demo_names_ghost_vblank(unsigned char strip_idx, unsigned char* oam_buf) {
    oam_write_strip_at(strip_idx, (unsigned char)(strip_idx << 4), oam_buf);
}

void render_demo_names_oam_flush(const unsigned char* oam_buf) {
    unsigned char i;
    OAM_ADDR_REG = 0x60;
    for (i = 0; i < 64; ++i) {
        OAM_DATA_REG = oam_buf[i];
    }
}

void render_demo_names_packet(unsigned char packet_idx) {
    render_demo_names_packet_vblank(packet_idx);
}

void render_chase_init(void) {
    unsigned char i;

    (void)PPU_STATUS_REG;

    PPU_ADDR_REG = 0x22;
    PPU_ADDR_REG = 0xAD;
    for (i = 0; i < 8; ++i) {
        PPU_DATA_REG = 0x2D;
    }

    PPU_ADDR_REG = 0x22;
    PPU_ADDR_REG = 0xED;
    for (i = 0; i < 8; ++i) {
        PPU_DATA_REG = 0x2D;
    }

    render_demo_scroll(0);
}
