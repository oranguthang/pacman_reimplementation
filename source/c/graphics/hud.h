// Some defines for the elements in the HUD
#define PRG_BANK_HUD 2

#define HUD_POSITION_START 0x0300
#define HUD_HEART_START 0x0361
#define HUD_KEY_START 0x037d
#define HUD_ATTRS_START 0x03f0

#define HUD_TILE_HEART 0xe7
#define HUD_TILE_HEART_EMPTY 0xe9
#define HUD_TILE_KEY 0xe8
#define HUD_TILE_NUMBER 0xf6
#define HUD_TILE_BLANK 0x20
#define HUD_TILE_BORDER_BL 0xf4
#define HUD_TILE_BORDER_BR 0xf5
#define HUD_TILE_BORDER_HORIZONTAL 0xe5
#define HUD_TILE_BORDER_VERTICAL 0xe4

#define HUD_SPRITE_ZERO_TILE_ID 0xfb

#define HUD_LABEL_HI_SCORE_ADDR  NTADR_A(22, 3)
#define HUD_HI_SCORE_ADDR        NTADR_A(23, 5)
#define HUD_LABEL_1UP_ADDR       NTADR_A(23, 7)
#define HUD_SCORE_ADDR           NTADR_A(23, 9)
#define HUD_LABEL_PAUSE_ADDR     NTADR_A(23, 17)
#define HUD_LIVES_ADDR           NTADR_A(23, 24)

// Draw the HUD
void draw_hud(void);

// Show or hide the in-game PAUSE label on the side panel.
void show_hud_pause(void);
void hide_hud_pause(void);

// Update score digits during gameplay.
void update_hud(void);
