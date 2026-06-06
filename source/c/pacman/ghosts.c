#include "source/c/pacman/ghosts.h"

#include "source/c/configuration/game_states.h"
#include "source/c/configuration/system_constants.h"
#include "source/c/globals.h"
#include "source/c/library/bank_helpers.h"
#include "source/c/map/map.h"
#include "source/c/neslib.h"
#include "source/c/pacman/pellets.h"
#include "source/c/pacman/score.h"
#include "source/c/sprites/map_sprites.h"
#include "source/c/sprites/player.h"
#include "source/c/sprites/sprite_definitions.h"

CODE_BANK(PRG_BANK_PACMAN_GHOSTS);

#define PACMAN_GHOST_COUNT              4
#define PACMAN_GHOST_SIZE               16
#define PACMAN_GHOST_COLLISION_INSET    4
#define PACMAN_GHOST_STATE_HOUSE        0
#define PACMAN_GHOST_STATE_EXIT         2
#define PACMAN_GHOST_STATE_ACTIVE       4
#define PACMAN_GHOST_STATE_EYES         6
#define PACMAN_GHOST_STATE_EATEN_PAUSE  8
#define PACMAN_GHOST_DIR_UP             0
#define PACMAN_GHOST_DIR_LEFT           1
#define PACMAN_GHOST_DIR_DOWN           2
#define PACMAN_GHOST_DIR_RIGHT          3
#define PACMAN_GHOST_SPEED_NORMAL       16
#define PACMAN_GHOST_SPEED_TUNNEL       8
#define PACMAN_GHOST_SPEED_FRIGHTENED   8
#define PACMAN_GHOST_SPEED_EYES         32
#define PACMAN_GHOST_SPEED_HOUSE        8
#define PACMAN_GHOST_HOME_X             92
#define PACMAN_GHOST_HOME_Y             108
#define PACMAN_GHOST_EXIT_Y             84
#define PACMAN_GHOST_TOP_HOUSE_Y        104
#define PACMAN_GHOST_BOTTOM_HOUSE_Y     112
#define PACMAN_GHOST_FRIGHTENED_FRAMES  360u
#define PACMAN_GHOST_FLASH_FRAMES       120u
#define PACMAN_GHOST_RELEASE_DELAY      180u
#define PACMAN_GHOST_MODE_COUNT         7
#define PACMAN_GHOST_PELLET_TOTAL       192
#define PACMAN_GHOST_TUNNEL_ROW         13
#define PACMAN_GHOST_MAZE_ROWS          27
#define PACMAN_GHOST_MAZE_COLS          22
#define PACMAN_GHOST_MAZE_ROW_OFFSET    2
#define PACMAN_GHOST_MAZE_COL_OFFSET    0
#define PACMAN_GHOST_RIGHT_WRAP_X       ((PACMAN_GHOST_MAZE_COLS - 1) << 3)
#define PACMAN_GHOST_GRID_SIZE          8
#define PACMAN_GHOST_GRID_MASK          (PACMAN_GHOST_GRID_SIZE - 1)
#define PACMAN_GHOST_GRID_ANCHOR        4
#define PACMAN_GHOST_OAM_PALETTE_0      0x00
#define PACMAN_GHOST_OAM_PALETTE_1      0x01
#define PACMAN_GHOST_OAM_PALETTE_2      0x02
#define PACMAN_GHOST_OAM_PALETTE_3      0x03
#define PACMAN_FRIGHTENED_PALETTE_ADDR  0x15
#define PACMAN_FRIGHTENED_COLOR_BLUE    0x11
#define PACMAN_FRIGHTENED_COLOR_WHITE   0x20
#define PACMAN_GHOST_EYES_TARGET_X      PACMAN_GHOST_HOME_X
#define PACMAN_GHOST_EYES_DOOR_TOP_Y    (PACMAN_GHOST_HOME_Y - 8)
#define PACMAN_GHOST_EYES_TARGET_Y      PACMAN_GHOST_EYES_DOOR_TOP_Y
#define PACMAN_GHOST_EYES_ENTRY_Y       PACMAN_GHOST_HOME_Y
#define PACMAN_GHOST_POST_EAT_FREEZE    40

static const unsigned int k_mode_durations[PACMAN_GHOST_MODE_COUNT] = {
    420u, 1200u, 420u, 1200u, 300u, 1200u, 300u
};

static const unsigned char k_release_pellet_thresholds[PACMAN_GHOST_COUNT] = {
    0, 0, 30, 90
};

static const unsigned int k_ghost_eat_scores[PACMAN_GHOST_COUNT] = {
    200u, 400u, 800u, 1600u
};

static const unsigned char k_corner_targets[PACMAN_GHOST_COUNT][2] = {
    { 168,   8 },
    {  24,   8 },
    { 168, 208 },
    {  24, 208 }
};

static const unsigned char k_spawn_x[PACMAN_GHOST_COUNT] = {
    92, 92, 76, 92
};

static const unsigned char k_spawn_y[PACMAN_GHOST_COUNT] = {
    84, 108, 108, 108
};

static const unsigned char k_spawn_state[PACMAN_GHOST_COUNT] = {
    PACMAN_GHOST_STATE_ACTIVE,
    PACMAN_GHOST_STATE_HOUSE,
    PACMAN_GHOST_STATE_HOUSE,
    PACMAN_GHOST_STATE_HOUSE
};

static const unsigned char k_spawn_dir[PACMAN_GHOST_COUNT] = {
    PACMAN_GHOST_DIR_LEFT,
    PACMAN_GHOST_DIR_UP,
    PACMAN_GHOST_DIR_DOWN,
    PACMAN_GHOST_DIR_UP
};

static const unsigned char k_ghost_palette_bits[PACMAN_GHOST_COUNT] = {
    PACMAN_GHOST_OAM_PALETTE_0,
    PACMAN_GHOST_OAM_PALETTE_1,
    PACMAN_GHOST_OAM_PALETTE_2,
    PACMAN_GHOST_OAM_PALETTE_3
};

static const unsigned int k_distance_square_lut[32] = {
      0,   1,   4,   9,  16,  25,  36,  49,
     64,  81, 100, 121, 144, 169, 196, 225,
    256, 289, 324, 361, 400, 441, 484, 529,
    576, 625, 676, 729, 784, 841, 900, 961
};

static const unsigned char k_ghost_tiles[4][2][4] = {
    {
        {0x18, 0x18, 0x19, 0x19},
        {0x18, 0x18, 0x1A, 0x1A}
    },
    {
        {0x1B, 0x1C, 0x1D, 0x1F},
        {0x1B, 0x1C, 0x1E, 0x20}
    },
    {
        {0x21, 0x21, 0x22, 0x22},
        {0x21, 0x21, 0x23, 0x23}
    },
    {
        {0x1C, 0x1B, 0x1F, 0x1D},
        {0x1C, 0x1B, 0x20, 0x1E}
    }
};

static const unsigned char k_ghost_attrs[4][2][4] = {
    {
        {0x00, 0x40, 0x00, 0x40},
        {0x00, 0x40, 0x00, 0x40}
    },
    {
        {0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00, 0x00}
    },
    {
        {0x00, 0x40, 0x00, 0x40},
        {0x00, 0x40, 0x00, 0x40}
    },
    {
        {0x40, 0x40, 0x40, 0x40},
        {0x40, 0x40, 0x40, 0x40}
    }
};

static const unsigned char k_frightened_tiles[2][4] = {
    {0x24, 0x24, 0x25, 0x25},
    {0x24, 0x24, 0x26, 0x26}
};

static const unsigned char k_frightened_attrs[2][4] = {
    {0x00, 0x40, 0x00, 0x40},
    {0x00, 0x40, 0x00, 0x40}
};

static const unsigned char k_eyes_tiles[4][4] = {
    {0x27, 0x27, 0x4C, 0x4C},
    {0x28, 0x29, 0x2A, 0x2B},
    {0x2C, 0x2C, 0x2D, 0x2D},
    {0x29, 0x28, 0x2B, 0x2A}
};

static const unsigned char k_eyes_attrs[4][4] = {
    {0x00, 0x40, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00},
    {0x00, 0x40, 0x00, 0x40},
    {0x40, 0x40, 0x40, 0x40}
};

static const unsigned char k_pacman_walkable_tiles[PACMAN_GHOST_MAZE_ROWS][PACMAN_GHOST_MAZE_COLS] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0},
    {0,0,1,0,0,0,1,0,0,0,1,0,1,0,0,0,1,0,0,0,1,0},
    {0,0,1,0,0,0,1,0,0,0,1,0,1,0,0,0,1,0,0,0,1,0},
    {0,0,1,0,0,0,1,0,0,0,1,0,1,0,0,0,1,0,0,0,1,0},
    {0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
    {0,0,1,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,1,0},
    {0,0,1,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,1,0},
    {0,0,1,1,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,1,1,0},
    {0,0,0,0,0,0,1,0,0,0,1,0,1,0,0,0,1,0,0,0,0,0},
    {0,0,0,0,0,0,1,0,1,1,1,1,1,1,1,0,1,0,0,0,0,0},
    {0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,0},
    {0,0,0,0,0,0,1,0,1,0,1,1,1,0,1,0,1,0,0,0,0,0},
    {0,0,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,0},
    {0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,0},
    {0,0,0,0,0,0,1,0,1,1,1,1,1,1,1,0,1,0,0,0,0,0},
    {0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,0},
    {0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,0},
    {0,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0},
    {0,0,1,0,0,0,1,0,0,0,1,0,1,0,0,0,1,0,0,0,1,0},
    {0,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,0},
    {0,0,0,0,1,0,1,0,1,0,0,0,0,0,1,0,1,0,1,0,0,0},
    {0,0,0,0,1,0,1,0,1,0,0,0,0,0,1,0,1,0,1,0,0,0},
    {0,0,1,1,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,1,1,0},
    {0,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,1,0},
    {0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

static unsigned char s_ghost_x[PACMAN_GHOST_COUNT];
static unsigned char s_ghost_y[PACMAN_GHOST_COUNT];
static unsigned char s_ghost_substep[PACMAN_GHOST_COUNT];
static unsigned char s_ghost_state[PACMAN_GHOST_COUNT];
static unsigned char s_ghost_direction[PACMAN_GHOST_COUNT];
static unsigned char s_mode_is_chase;
static unsigned char s_mode_phase;
static unsigned char s_release_index;
static unsigned char s_kill_chain;
static unsigned char s_frightened_palette_color;
static unsigned char s_post_eat_freeze_timer;
static unsigned char s_post_eat_slot;
static unsigned int s_mode_timer;
static unsigned int s_release_timer;
static unsigned int s_frightened_timer;

static unsigned char abs_delta(unsigned char lhs, unsigned char rhs) {
    if (lhs >= rhs) {
        return lhs - rhs;
    }

    return rhs - lhs;
}

static unsigned char ghost_center_x(unsigned char slot) {
    return s_ghost_x[slot] + 8;
}

static unsigned char ghost_center_y(unsigned char slot) {
    return s_ghost_y[slot] + 8;
}

static unsigned char tile_x_from_pixel(unsigned char pixel_x) {
    return pixel_x >> 3;
}

static unsigned char tile_y_from_pixel(unsigned char pixel_y) {
    return pixel_y >> 3;
}

static unsigned char player_center_x(void) {
    return (unsigned char)((playerXPosition >> PLAYER_POSITION_SHIFT) + 8);
}

static unsigned char player_center_y(void) {
    return (unsigned char)((playerYPosition >> PLAYER_POSITION_SHIFT) + 8);
}

static unsigned char player_direction_to_ghost_direction(void) {
    switch (playerDirection) {
        case SPRITE_DIRECTION_UP:
            return PACMAN_GHOST_DIR_UP;
        case SPRITE_DIRECTION_LEFT:
            return PACMAN_GHOST_DIR_LEFT;
        case SPRITE_DIRECTION_RIGHT:
            return PACMAN_GHOST_DIR_RIGHT;
        case SPRITE_DIRECTION_DOWN:
        default:
            return PACMAN_GHOST_DIR_DOWN;
    }
}

static unsigned char opposite_direction(unsigned char direction) {
    return (direction + 2) & 0x03;
}

static unsigned char pacman_maze_walkable_pixel(unsigned char x, unsigned char y) {
    unsigned char tile_row = y >> 3;
    unsigned char tile_col = x >> 3;

    if (tile_row < PACMAN_GHOST_MAZE_ROW_OFFSET || tile_row >= (PACMAN_GHOST_MAZE_ROW_OFFSET + PACMAN_GHOST_MAZE_ROWS)) {
        return 0;
    }
    if (tile_row == (PACMAN_GHOST_MAZE_ROW_OFFSET + PACMAN_GHOST_TUNNEL_ROW) &&
        (tile_col < 2 || tile_col >= 21)) {
        return 1;
    }
    if (tile_col >= (PACMAN_GHOST_MAZE_COL_OFFSET + PACMAN_GHOST_MAZE_COLS)) {
        return 0;
    }

    return k_pacman_walkable_tiles[tile_row - PACMAN_GHOST_MAZE_ROW_OFFSET][tile_col - PACMAN_GHOST_MAZE_COL_OFFSET];
}

static unsigned char can_move_in_direction_from(unsigned char direction, unsigned char x_position, unsigned char y_position) {
    unsigned char center_x = x_position + 8;
    unsigned char center_y = y_position + 8;

    switch (direction) {
        case PACMAN_GHOST_DIR_LEFT:
            if ((center_y >> 3) == (PACMAN_GHOST_MAZE_ROW_OFFSET + PACMAN_GHOST_TUNNEL_ROW) && center_x <= 16) {
                return 1;
            }
            return pacman_maze_walkable_pixel((unsigned char)(center_x - 8), center_y);
        case PACMAN_GHOST_DIR_RIGHT:
            if ((center_y >> 3) == (PACMAN_GHOST_MAZE_ROW_OFFSET + PACMAN_GHOST_TUNNEL_ROW) && center_x >= PACMAN_GHOST_RIGHT_WRAP_X) {
                return 1;
            }
            return pacman_maze_walkable_pixel((unsigned char)(center_x + 8), center_y);
        case PACMAN_GHOST_DIR_UP:
            return pacman_maze_walkable_pixel(center_x, (unsigned char)(center_y - 8));
        case PACMAN_GHOST_DIR_DOWN:
            return pacman_maze_walkable_pixel(center_x, (unsigned char)(center_y + 8));
        default:
            return 0;
    }
}

static unsigned char ghost_is_in_tunnel_row(unsigned char slot) {
    unsigned char center_y = s_ghost_y[slot] + 8;
    return (center_y >> 3) == (PACMAN_GHOST_MAZE_ROW_OFFSET + PACMAN_GHOST_TUNNEL_ROW);
}

static unsigned char can_snap_to_grid(unsigned char position) {
    unsigned char offset = (unsigned char)((position - PACMAN_GHOST_GRID_ANCHOR) & PACMAN_GHOST_GRID_MASK);
    return offset == 0;
}

static unsigned char movement_pixels_for_slot(unsigned char slot, unsigned char speed) {
    unsigned char total = (unsigned char)(s_ghost_substep[slot] + speed);
    s_ghost_substep[slot] = total & ((1 << PLAYER_POSITION_SHIFT) - 1);
    return total >> PLAYER_POSITION_SHIFT;
}

static void set_frightened_palette_color(unsigned char color) {
    if (s_frightened_palette_color == color) {
        return;
    }

    s_frightened_palette_color = color;
    pal_col(PACMAN_FRIGHTENED_PALETTE_ADDR, color);
}

static void update_frightened_palette_phase(void) {
    if (s_frightened_timer == 0u) {
        set_frightened_palette_color(PACMAN_FRIGHTENED_COLOR_BLUE);
        return;
    }

    if (s_frightened_timer <= PACMAN_GHOST_FLASH_FRAMES) {
        if ((frameCount & 0x08) == 0u) {
            set_frightened_palette_color(PACMAN_FRIGHTENED_COLOR_WHITE);
        } else {
            set_frightened_palette_color(PACMAN_FRIGHTENED_COLOR_BLUE);
        }
        return;
    }

    set_frightened_palette_color(PACMAN_FRIGHTENED_COLOR_BLUE);
}

static unsigned char ghost_speed_for_slot(unsigned char slot) {
    if (s_ghost_state[slot] == PACMAN_GHOST_STATE_HOUSE || s_ghost_state[slot] == PACMAN_GHOST_STATE_EXIT) {
        return PACMAN_GHOST_SPEED_HOUSE;
    }
    if (s_ghost_state[slot] == PACMAN_GHOST_STATE_EYES) {
        return PACMAN_GHOST_SPEED_EYES;
    }
    if (s_frightened_timer != 0u) {
        return PACMAN_GHOST_SPEED_FRIGHTENED;
    }
    if (ghost_is_in_tunnel_row(slot)) {
        return PACMAN_GHOST_SPEED_TUNNEL;
    }

    return PACMAN_GHOST_SPEED_NORMAL;
}

static void hide_ghost(unsigned char slot) {
    unsigned char oam_index = FIRST_ENEMY_SPRITE_OAM_INDEX + (slot << 4);

    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0x00, 0x00, oam_index);
    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0x00, 0x00, oam_index + 4);
    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0x00, 0x00, oam_index + 8);
    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0x00, 0x00, oam_index + 12);
}

static void draw_ghost(unsigned char slot) {
    unsigned char oam_index = FIRST_ENEMY_SPRITE_OAM_INDEX + (slot << 4);
    unsigned char pixel_x = s_ghost_x[slot];
    unsigned char pixel_y = s_ghost_y[slot];
    unsigned char frame = (frameCount >> 3) & 0x01;
    const unsigned char* tiles;
    const unsigned char* attrs;
    unsigned char palette_bits;

    if (s_ghost_state[slot] == PACMAN_GHOST_STATE_EATEN_PAUSE) {
        hide_ghost(slot);
        return;
    }

    if (s_ghost_state[slot] == PACMAN_GHOST_STATE_EYES) {
        tiles = k_eyes_tiles[s_ghost_direction[slot] & 0x03];
        attrs = k_eyes_attrs[s_ghost_direction[slot] & 0x03];
        palette_bits = PACMAN_GHOST_OAM_PALETTE_1;
    } else if (s_ghost_state[slot] == PACMAN_GHOST_STATE_ACTIVE && s_frightened_timer != 0u) {
        tiles = k_frightened_tiles[frame];
        attrs = k_frightened_attrs[frame];
        palette_bits = PACMAN_GHOST_OAM_PALETTE_1;
    } else {
        tiles = k_ghost_tiles[s_ghost_direction[slot] & 0x03][frame];
        attrs = k_ghost_attrs[s_ghost_direction[slot] & 0x03][frame];
        palette_bits = k_ghost_palette_bits[slot];
    }

    oam_spr(pixel_x, pixel_y, tiles[0], palette_bits | attrs[0], oam_index);
    oam_spr(pixel_x + NES_SPRITE_WIDTH, pixel_y, tiles[1], palette_bits | attrs[1], oam_index + 4);
    oam_spr(pixel_x, pixel_y + NES_SPRITE_HEIGHT, tiles[2], palette_bits | attrs[2], oam_index + 8);
    oam_spr(pixel_x + NES_SPRITE_WIDTH, pixel_y + NES_SPRITE_HEIGHT, tiles[3], palette_bits | attrs[3], oam_index + 12);
}

static void reverse_active_ghosts(void) {
    unsigned char slot;

    for (slot = 0; slot < PACMAN_GHOST_COUNT; ++slot) {
        if (s_ghost_state[slot] == PACMAN_GHOST_STATE_ACTIVE) {
            s_ghost_direction[slot] = opposite_direction(s_ghost_direction[slot]);
        }
    }
}

static void target_for_slot(unsigned char slot, unsigned char* target_x, unsigned char* target_y) {
    unsigned char player_x = player_center_x();
    unsigned char player_y = player_center_y();
    unsigned char player_dir = player_direction_to_ghost_direction();

    if (s_ghost_state[slot] == PACMAN_GHOST_STATE_EYES) {
        *target_x = PACMAN_GHOST_EYES_TARGET_X;
        *target_y = PACMAN_GHOST_EYES_TARGET_Y;
        return;
    }

    if (!s_mode_is_chase) {
        *target_x = k_corner_targets[slot][0];
        *target_y = k_corner_targets[slot][1];
        return;
    }

    switch (slot) {
        case 0:
            *target_x = player_x;
            *target_y = player_y;
            break;
        case 1:
            *target_x = player_x;
            *target_y = player_y;
            switch (player_dir) {
                case PACMAN_GHOST_DIR_UP:
                    *target_y = (unsigned char)(player_y - 24);
                    break;
                case PACMAN_GHOST_DIR_LEFT:
                    *target_x = (unsigned char)(player_x - 24);
                    break;
                case PACMAN_GHOST_DIR_RIGHT:
                    *target_x = (unsigned char)(player_x + 24);
                    break;
                case PACMAN_GHOST_DIR_DOWN:
                default:
                    *target_y = (unsigned char)(player_y + 24);
                    break;
            }
            break;
        case 2:
            *target_x = (unsigned char)(player_x + (player_x - ghost_center_x(0)));
            *target_y = (unsigned char)(player_y + (player_y - ghost_center_y(0)));
            break;
        case 3:
            if (abs_delta(player_x, ghost_center_x(slot)) < 32 &&
                abs_delta(player_y, ghost_center_y(slot)) < 32) {
                *target_x = k_corner_targets[slot][0];
                *target_y = k_corner_targets[slot][1];
            } else {
                *target_x = player_x;
                *target_y = player_y;
            }
            break;
    }
}

static unsigned char choose_random_direction(unsigned char slot) {
    unsigned char start = rand8() & 0x03;
    unsigned char dir;
    unsigned char reverse = opposite_direction(s_ghost_direction[slot]);

    for (dir = 0; dir < 4; ++dir) {
        unsigned char candidate = (start + dir) & 0x03;
        if (candidate == reverse &&
            s_ghost_state[slot] != PACMAN_GHOST_STATE_EYES) {
            continue;
        }
        if (can_move_in_direction_from(candidate, s_ghost_x[slot], s_ghost_y[slot])) {
            return candidate;
        }
    }

    if (can_move_in_direction_from(reverse, s_ghost_x[slot], s_ghost_y[slot])) {
        return reverse;
    }

    return s_ghost_direction[slot];
}

static unsigned char choose_targeted_direction(unsigned char slot) {
    unsigned char target_x;
    unsigned char target_y;
    unsigned char target_tile_x;
    unsigned char target_tile_y;
    unsigned char candidate;
    unsigned char chosen = 0xFF;
    unsigned int best_distance = 0xFFFFu;
    unsigned char reverse = opposite_direction(s_ghost_direction[slot]);

    target_for_slot(slot, &target_x, &target_y);
    target_tile_x = tile_x_from_pixel(target_x);
    target_tile_y = tile_y_from_pixel(target_y);

    for (candidate = 0; candidate < 4; ++candidate) {
        unsigned char candidate_tile_x = tile_x_from_pixel((unsigned char)(s_ghost_x[slot] + 8));
        unsigned char candidate_tile_y = tile_y_from_pixel((unsigned char)(s_ghost_y[slot] + 8));
        unsigned char delta_x;
        unsigned char delta_y;
        unsigned int distance;

        if (candidate == reverse) {
            continue;
        }
        if (!can_move_in_direction_from(candidate, s_ghost_x[slot], s_ghost_y[slot])) {
            continue;
        }

        switch (candidate) {
            case PACMAN_GHOST_DIR_UP:
                --candidate_tile_y;
                break;
            case PACMAN_GHOST_DIR_LEFT:
                --candidate_tile_x;
                break;
            case PACMAN_GHOST_DIR_DOWN:
                ++candidate_tile_y;
                break;
            case PACMAN_GHOST_DIR_RIGHT:
                ++candidate_tile_x;
                break;
        }
        delta_x = abs_delta(candidate_tile_x, target_tile_x);
        delta_y = abs_delta(candidate_tile_y, target_tile_y);
        distance = k_distance_square_lut[delta_x] + k_distance_square_lut[delta_y];

        if (chosen == 0xFF || distance < best_distance) {
            chosen = candidate;
            best_distance = distance;
        }
    }

    if (chosen != 0xFF) {
        return chosen;
    }

    if (can_move_in_direction_from(reverse, s_ghost_x[slot], s_ghost_y[slot])) {
        return reverse;
    }

    return s_ghost_direction[slot];
}

static void choose_direction_if_needed(unsigned char slot) {
    if (s_ghost_state[slot] != PACMAN_GHOST_STATE_ACTIVE &&
        s_ghost_state[slot] != PACMAN_GHOST_STATE_EYES) {
        return;
    }

    if (!can_snap_to_grid(s_ghost_x[slot]) || !can_snap_to_grid(s_ghost_y[slot])) {
        return;
    }

    if (s_frightened_timer != 0u && s_ghost_state[slot] == PACMAN_GHOST_STATE_ACTIVE) {
        s_ghost_direction[slot] = choose_random_direction(slot);
    } else {
        s_ghost_direction[slot] = choose_targeted_direction(slot);
    }
}

static void move_ghost(unsigned char slot) {
    unsigned char speed = ghost_speed_for_slot(slot);
    unsigned char pixels = movement_pixels_for_slot(slot, speed);

    while (pixels != 0) {
        if (can_snap_to_grid(s_ghost_x[slot]) && can_snap_to_grid(s_ghost_y[slot]) &&
            !can_move_in_direction_from(s_ghost_direction[slot], s_ghost_x[slot], s_ghost_y[slot])) {
            if (s_ghost_state[slot] == PACMAN_GHOST_STATE_ACTIVE ||
                s_ghost_state[slot] == PACMAN_GHOST_STATE_EYES) {
                s_ghost_direction[slot] = choose_targeted_direction(slot);
            }
            if (!can_move_in_direction_from(s_ghost_direction[slot], s_ghost_x[slot], s_ghost_y[slot])) {
                break;
            }
        }

        switch (s_ghost_direction[slot]) {
            case PACMAN_GHOST_DIR_UP:
                --s_ghost_y[slot];
                break;
            case PACMAN_GHOST_DIR_LEFT:
                --s_ghost_x[slot];
                break;
            case PACMAN_GHOST_DIR_DOWN:
                ++s_ghost_y[slot];
                break;
            case PACMAN_GHOST_DIR_RIGHT:
                ++s_ghost_x[slot];
                break;
        }

        if (ghost_is_in_tunnel_row(slot) && s_ghost_x[slot] > PACMAN_GHOST_RIGHT_WRAP_X) {
            if (s_ghost_direction[slot] == PACMAN_GHOST_DIR_LEFT) {
                s_ghost_x[slot] = PACMAN_GHOST_RIGHT_WRAP_X;
            } else if (s_ghost_direction[slot] == PACMAN_GHOST_DIR_RIGHT) {
                s_ghost_x[slot] = 0;
            }
        }

        --pixels;
    }
}

static unsigned char ghost_collides_player(unsigned char slot) {
    unsigned char player_left = (unsigned char)((playerXPosition + PLAYER_X_OFFSET_EXTENDED) >> PLAYER_POSITION_SHIFT);
    unsigned char player_top = (unsigned char)((playerYPosition + PLAYER_Y_OFFSET_EXTENDED) >> PLAYER_POSITION_SHIFT);
    unsigned char player_right = player_left + (unsigned char)(PLAYER_WIDTH_EXTENDED >> PLAYER_POSITION_SHIFT);
    unsigned char player_bottom = player_top + (unsigned char)(PLAYER_HEIGHT_EXTENDED >> PLAYER_POSITION_SHIFT);
    unsigned char ghost_left = s_ghost_x[slot] + 2;
    unsigned char ghost_top = s_ghost_y[slot] + 2;
    unsigned char ghost_right = ghost_left + 12;
    unsigned char ghost_bottom = ghost_top + 12;

    return player_left < ghost_right &&
           player_right > ghost_left &&
           player_top < ghost_bottom &&
           player_bottom > ghost_top;
}

static void handle_ghost_collision(unsigned char slot) {
    if (!ghost_collides_player(slot)) {
        return;
    }

    if (s_ghost_state[slot] == PACMAN_GHOST_STATE_ACTIVE && s_frightened_timer != 0u) {
        unsigned char score_index = s_kill_chain;

        if (score_index >= PACMAN_GHOST_COUNT) {
            score_index = PACMAN_GHOST_COUNT - 1;
        }

        pacman_score_add(k_ghost_eat_scores[score_index]);
        if (s_kill_chain < PACMAN_GHOST_COUNT - 1) {
            ++s_kill_chain;
        }
        s_ghost_state[slot] = PACMAN_GHOST_STATE_EATEN_PAUSE;
        s_ghost_substep[slot] = 0;
        s_post_eat_slot = slot;
        s_post_eat_freeze_timer = PACMAN_GHOST_POST_EAT_FREEZE;
        return;
    }

    if (s_ghost_state[slot] != PACMAN_GHOST_STATE_ACTIVE) {
        return;
    }

    gameState = GAME_STATE_GAME_OVER;
    music_stop();
    sfx_play(SFX_GAMEOVER, SFX_CHANNEL_1);
}

static void update_house_ghost(unsigned char slot) {
    unsigned char pixels = movement_pixels_for_slot(slot, PACMAN_GHOST_SPEED_HOUSE);

    while (pixels != 0) {
        if (s_ghost_direction[slot] == PACMAN_GHOST_DIR_UP) {
            --s_ghost_y[slot];
            if (s_ghost_y[slot] <= PACMAN_GHOST_TOP_HOUSE_Y) {
                s_ghost_y[slot] = PACMAN_GHOST_TOP_HOUSE_Y;
                s_ghost_direction[slot] = PACMAN_GHOST_DIR_DOWN;
            }
        } else {
            ++s_ghost_y[slot];
            if (s_ghost_y[slot] >= PACMAN_GHOST_BOTTOM_HOUSE_Y) {
                s_ghost_y[slot] = PACMAN_GHOST_BOTTOM_HOUSE_Y;
                s_ghost_direction[slot] = PACMAN_GHOST_DIR_UP;
            }
        }

        --pixels;
    }
}

static void update_exit_ghost(unsigned char slot) {
    unsigned char pixels = movement_pixels_for_slot(slot, PACMAN_GHOST_SPEED_HOUSE);

    while (pixels != 0) {
        if (s_ghost_x[slot] < PACMAN_GHOST_HOME_X) {
            s_ghost_direction[slot] = PACMAN_GHOST_DIR_RIGHT;
            ++s_ghost_x[slot];
            if (s_ghost_x[slot] > PACMAN_GHOST_HOME_X) {
                s_ghost_x[slot] = PACMAN_GHOST_HOME_X;
            }
            --pixels;
            continue;
        }
        if (s_ghost_x[slot] > PACMAN_GHOST_HOME_X) {
            s_ghost_direction[slot] = PACMAN_GHOST_DIR_LEFT;
            --s_ghost_x[slot];
            if (s_ghost_x[slot] < PACMAN_GHOST_HOME_X) {
                s_ghost_x[slot] = PACMAN_GHOST_HOME_X;
            }
            --pixels;
            continue;
        }
        if (s_ghost_y[slot] > PACMAN_GHOST_EXIT_Y) {
            s_ghost_direction[slot] = PACMAN_GHOST_DIR_UP;
            --s_ghost_y[slot];
            if (s_ghost_y[slot] <= PACMAN_GHOST_EXIT_Y) {
                s_ghost_y[slot] = PACMAN_GHOST_EXIT_Y;
                s_ghost_state[slot] = PACMAN_GHOST_STATE_ACTIVE;
                s_ghost_direction[slot] = PACMAN_GHOST_DIR_LEFT;
                s_ghost_substep[slot] = 0;
            }
            --pixels;
            continue;
        }

        s_ghost_state[slot] = PACMAN_GHOST_STATE_ACTIVE;
        s_ghost_direction[slot] = PACMAN_GHOST_DIR_LEFT;
        s_ghost_substep[slot] = 0;
        break;
    }
}

static void update_eyes_ghost(unsigned char slot) {
    unsigned char pixels = movement_pixels_for_slot(slot, PACMAN_GHOST_SPEED_EYES);

    if (abs_delta(s_ghost_y[slot], PACMAN_GHOST_EYES_TARGET_Y) <= 2 &&
        s_ghost_x[slot] != PACMAN_GHOST_EYES_TARGET_X) {
        s_ghost_y[slot] = PACMAN_GHOST_EYES_TARGET_Y;

        if (s_ghost_x[slot] < PACMAN_GHOST_EYES_TARGET_X) {
            s_ghost_direction[slot] = PACMAN_GHOST_DIR_RIGHT;
            while (pixels != 0u && s_ghost_x[slot] < PACMAN_GHOST_EYES_TARGET_X) {
                ++s_ghost_x[slot];
                --pixels;
            }
        } else {
            s_ghost_direction[slot] = PACMAN_GHOST_DIR_LEFT;
            while (pixels != 0u && s_ghost_x[slot] > PACMAN_GHOST_EYES_TARGET_X) {
                --s_ghost_x[slot];
                --pixels;
            }
        }
    }

    if (abs_delta(s_ghost_x[slot], PACMAN_GHOST_EYES_TARGET_X) <= 2 &&
        s_ghost_y[slot] >= (PACMAN_GHOST_EYES_TARGET_Y - 2) &&
        s_ghost_y[slot] < PACMAN_GHOST_EYES_ENTRY_Y) {
        s_ghost_x[slot] = PACMAN_GHOST_EYES_TARGET_X;
        s_ghost_direction[slot] = PACMAN_GHOST_DIR_DOWN;

        while (pixels != 0u && s_ghost_y[slot] < PACMAN_GHOST_EYES_ENTRY_Y) {
            ++s_ghost_y[slot];
            --pixels;
        }

        if (s_ghost_y[slot] >= PACMAN_GHOST_EYES_ENTRY_Y) {
            s_ghost_y[slot] = PACMAN_GHOST_EYES_ENTRY_Y;
            s_ghost_state[slot] = PACMAN_GHOST_STATE_EXIT;
            s_ghost_direction[slot] = PACMAN_GHOST_DIR_UP;
            s_ghost_substep[slot] = 0;
        }
        return;
    }

    if (s_ghost_y[slot] >= (PACMAN_GHOST_EYES_DOOR_TOP_Y - 2) &&
        s_ghost_y[slot] < PACMAN_GHOST_EYES_ENTRY_Y &&
        abs_delta(s_ghost_x[slot], PACMAN_GHOST_HOME_X) <= 8) {
        if (s_ghost_x[slot] < PACMAN_GHOST_HOME_X) {
            s_ghost_direction[slot] = PACMAN_GHOST_DIR_RIGHT;
            ++s_ghost_x[slot];
        } else if (s_ghost_x[slot] > PACMAN_GHOST_HOME_X) {
            s_ghost_direction[slot] = PACMAN_GHOST_DIR_LEFT;
            --s_ghost_x[slot];
        } else {
            s_ghost_direction[slot] = PACMAN_GHOST_DIR_DOWN;
            ++s_ghost_y[slot];
        }

        if (s_ghost_y[slot] >= PACMAN_GHOST_EYES_ENTRY_Y) {
            s_ghost_x[slot] = PACMAN_GHOST_HOME_X;
            s_ghost_y[slot] = PACMAN_GHOST_EYES_ENTRY_Y;
            s_ghost_state[slot] = PACMAN_GHOST_STATE_EXIT;
            s_ghost_direction[slot] = PACMAN_GHOST_DIR_UP;
            s_ghost_substep[slot] = 0;
        }
        return;
    }

    choose_direction_if_needed(slot);
    move_ghost(slot);

    if (abs_delta(s_ghost_x[slot], PACMAN_GHOST_HOME_X) <= 1 &&
        s_ghost_y[slot] == PACMAN_GHOST_EYES_ENTRY_Y) {
        s_ghost_x[slot] = PACMAN_GHOST_HOME_X;
        s_ghost_state[slot] = PACMAN_GHOST_STATE_EXIT;
        s_ghost_direction[slot] = PACMAN_GHOST_DIR_UP;
        s_ghost_substep[slot] = 0;
    }
}

static void update_active_ghost(unsigned char slot) {
    choose_direction_if_needed(slot);
    move_ghost(slot);
    handle_ghost_collision(slot);
}

static void try_release_next_ghost(void) {
    unsigned char pellets_eaten;

    if (s_release_index >= PACMAN_GHOST_COUNT) {
        return;
    }

    pellets_eaten = PACMAN_GHOST_PELLET_TOTAL - pacmanPelletsRemaining;

    if (pellets_eaten >= k_release_pellet_thresholds[s_release_index] || s_release_timer == 0u) {
        s_ghost_state[s_release_index] = PACMAN_GHOST_STATE_EXIT;
        s_ghost_substep[s_release_index] = 0;
        s_release_index++;
        s_release_timer = PACMAN_GHOST_RELEASE_DELAY;
    }
}

static void update_mode_timers(void) {
    if (s_frightened_timer != 0u) {
        --s_frightened_timer;
        if (s_frightened_timer == 0u) {
            s_kill_chain = 0;
        }
        return;
    }

    if (s_mode_phase >= PACMAN_GHOST_MODE_COUNT) {
        return;
    }

    if (s_mode_timer != 0u) {
        --s_mode_timer;
    }
    if (s_mode_timer == 0u) {
        s_mode_is_chase = !s_mode_is_chase;
        ++s_mode_phase;
        if (s_mode_phase < PACMAN_GHOST_MODE_COUNT) {
            s_mode_timer = k_mode_durations[s_mode_phase];
            reverse_active_ghosts();
        }
    }
}

void pacman_ghosts_init(void) {
    unsigned char slot;

    s_mode_is_chase = 0;
    s_mode_phase = 0;
    s_mode_timer = k_mode_durations[0];
    s_release_index = 1;
    s_release_timer = PACMAN_GHOST_RELEASE_DELAY;
    s_frightened_timer = 0u;
    s_kill_chain = 0;
    s_frightened_palette_color = 0xff;
    s_post_eat_freeze_timer = 0;
    s_post_eat_slot = 0xff;
    set_frightened_palette_color(PACMAN_FRIGHTENED_COLOR_BLUE);

    for (slot = 0; slot < PACMAN_GHOST_COUNT; ++slot) {
        s_ghost_x[slot] = k_spawn_x[slot];
        s_ghost_y[slot] = k_spawn_y[slot];
        s_ghost_substep[slot] = 0;
        s_ghost_state[slot] = k_spawn_state[slot];
        s_ghost_direction[slot] = k_spawn_dir[slot];
        draw_ghost(slot);
    }

    for (; slot < MAP_MAX_SPRITES; ++slot) {
        hide_ghost(slot);
    }
}

void pacman_ghosts_start_frightened(void) {
    s_frightened_timer = PACMAN_GHOST_FRIGHTENED_FRAMES;
    s_kill_chain = 0;
    set_frightened_palette_color(PACMAN_FRIGHTENED_COLOR_BLUE);
    reverse_active_ghosts();
}

unsigned char pacman_ghosts_is_post_eat_freeze_active(void) {
    return s_post_eat_freeze_timer != 0u;
}

void pacman_ghosts_update(void) {
    unsigned char slot;

    if (s_post_eat_freeze_timer != 0u) {
        --s_post_eat_freeze_timer;
        if (s_post_eat_freeze_timer == 0u &&
            s_post_eat_slot < PACMAN_GHOST_COUNT &&
            s_ghost_state[s_post_eat_slot] == PACMAN_GHOST_STATE_EATEN_PAUSE) {
            s_ghost_state[s_post_eat_slot] = PACMAN_GHOST_STATE_EYES;
            s_ghost_substep[s_post_eat_slot] = 0;
            s_post_eat_slot = 0xff;
        }

        for (slot = 0; slot < PACMAN_GHOST_COUNT; ++slot) {
            if (s_ghost_state[slot] == PACMAN_GHOST_STATE_EYES) {
                update_eyes_ghost(slot);
            }
        }

        for (slot = 0; slot < PACMAN_GHOST_COUNT; ++slot) {
            draw_ghost(slot);
        }
        return;
    }

    update_mode_timers();
    update_frightened_palette_phase();

    if (s_frightened_timer == 0u && s_release_timer != 0u) {
        --s_release_timer;
    }
    if (s_frightened_timer == 0u) {
        try_release_next_ghost();
    }

    for (slot = 0; slot < PACMAN_GHOST_COUNT; ++slot) {
        switch (s_ghost_state[slot]) {
            case PACMAN_GHOST_STATE_HOUSE:
                update_house_ghost(slot);
                break;
            case PACMAN_GHOST_STATE_EXIT:
                update_exit_ghost(slot);
                break;
            case PACMAN_GHOST_STATE_EYES:
                update_eyes_ghost(slot);
                break;
            case PACMAN_GHOST_STATE_ACTIVE:
            default:
                update_active_ghost(slot);
                break;
        }

        if (gameState != GAME_STATE_RUNNING) {
            break;
        }
    }

    for (slot = 0; slot < PACMAN_GHOST_COUNT; ++slot) {
        draw_ghost(slot);
    }
}
