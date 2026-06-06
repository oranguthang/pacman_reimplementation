#include "source/c/neslib.h"
#include "source/c/sprites/player.h"
#include "source/c/library/bank_helpers.h"
#include "source/c/globals.h"
#include "source/c/map/map.h"
#include "source/c/configuration/game_states.h"
#include "source/c/configuration/system_constants.h"
#include "source/c/sprites/collision.h"
#include "source/c/sprites/sprite_definitions.h"
#include "source/c/sprites/map_sprites.h"
#include "source/c/menus/error.h"
#include "source/c/graphics/hud.h"
#include "source/c/graphics/game_text.h"
#include "source/c/sprites/map_sprites.h"

CODE_BANK(PRG_BANK_PLAYER_SPRITE);

// Some useful global variables
ZEROPAGE_DEF(int, playerXPosition);
ZEROPAGE_DEF(int, playerYPosition);
ZEROPAGE_DEF(int, playerXVelocity);
ZEROPAGE_DEF(int, playerYVelocity);
ZEROPAGE_DEF(int, nextPlayerXPosition);
ZEROPAGE_DEF(int, nextPlayerYPosition);
ZEROPAGE_DEF(unsigned char, playerControlsLockTime);
ZEROPAGE_DEF(unsigned char, playerInvulnerabilityTime);
ZEROPAGE_DEF(unsigned char, playerDirection);

// Huge pile of temporary variables
#define rawXPosition tempChar1
#define rawYPosition tempChar2
#define rawTileId tempChar3
#define collisionTempX tempChar4
#define collisionTempY tempChar5
#define collisionTempXRight tempChar6
#define collisionTempYBottom tempChar7

#define tempSpriteCollisionX tempInt1
#define tempSpriteCollisionY tempInt2

#define collisionTempXInt tempInt3
#define collisionTempYInt tempInt4

 const unsigned char* introductionText = 
                                "Welcome to nes-starter-kit! I " 
                                "am an NPC.                    "
                                "                              "

                                "Hope you're having fun!       "
                                "                              "
                                "- The Developer";
const unsigned char* movedText = 
                                "Hey, you put me on another    "
                                "screen! Cool!";

static const unsigned char k_pacman_sprite_tiles[4][2][4] = {
    /* Up: indices 02/03 from tbl_DB59_actor_sprite_tiles */
    {
        {0x04, 0x04, 0x03, 0x03},
        {0x08, 0x08, 0x07, 0x07}
    },
    /* Left: indices 04/05 */
    {
        {0x02, 0x01, 0x02, 0x01},
        {0x06, 0x05, 0x06, 0x05}
    },
    /* Down: indices 06/07 */
    {
        {0x03, 0x03, 0x04, 0x04},
        {0x07, 0x07, 0x08, 0x08}
    },
    /* Right: indices 08/09 */
    {
        {0x01, 0x02, 0x01, 0x02},
        {0x05, 0x06, 0x05, 0x06}
    }
};

static const unsigned char k_pacman_sprite_attrs[4][2][4] = {
    {
        {0x80, 0xC0, 0x80, 0xC0},
        {0x80, 0xC0, 0x80, 0xC0}
    },
    {
        {0x00, 0x00, 0x80, 0x80},
        {0x00, 0x00, 0x80, 0x80}
    },
    {
        {0x00, 0x40, 0x00, 0x40},
        {0x00, 0x40, 0x00, 0x40}
    },
    {
        {0x40, 0x40, 0xC0, 0xC0},
        {0x40, 0x40, 0xC0, 0xC0}
    }
};

static const unsigned char k_pacman_closed_sprite_tiles[4] = {
    0x00, 0x00, 0x00, 0x00
};

static const unsigned char k_pacman_closed_sprite_attrs[4] = {
    0x00, 0x40, 0x80, 0xC0
};

static const unsigned char k_pacman_anim_sequence[8] = {
    0, 0, 0, 1, 1, 0, 2, 2
};

static unsigned char s_queued_direction = SPRITE_DIRECTION_STATIONARY;
static unsigned char s_animation_frame = 0;

#define PLAYER_MOVE_SPEED 16
#define PLAYER_GRID_SIZE_EXTENDED (8 << PLAYER_POSITION_SHIFT)
#define PLAYER_GRID_MASK_EXTENDED (PLAYER_GRID_SIZE_EXTENDED - 1)
#define PLAYER_GRID_ANCHOR_EXTENDED (4 << PLAYER_POSITION_SHIFT)
#define PLAYER_TURN_SNAP_THRESHOLD PLAYER_MOVE_SPEED
#define PLAYER_COLLISION_INSET 4
#define PLAYER_PROBE_LEFT_OFFSET 5
#define PLAYER_PROBE_RIGHT_OFFSET 4
#define PLAYER_PROBE_UP_OFFSET 5
#define PLAYER_PROBE_DOWN_OFFSET 4
#define PACMAN_TUNNEL_ROW 13
#define PACMAN_TUNNEL_RIGHT_WRAP_X (((PACMAN_MAZE_COLS - 1) << 3) << PLAYER_POSITION_SHIFT)

#define PACMAN_MAZE_ROWS 27
#define PACMAN_MAZE_COLS 22
#define PACMAN_MAZE_ROW_OFFSET 2
#define PACMAN_MAZE_COL_OFFSET 0

static const unsigned char k_pacman_walkable_tiles[PACMAN_MAZE_ROWS][PACMAN_MAZE_COLS] = {
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

static unsigned char get_pacman_direction_index(void) {
    switch (playerDirection) {
        case SPRITE_DIRECTION_UP:
            return 0;
        case SPRITE_DIRECTION_LEFT:
            return 1;
        case SPRITE_DIRECTION_RIGHT:
            return 3;
        case SPRITE_DIRECTION_DOWN:
        default:
            return 2;
    }
}

static unsigned char get_requested_direction(void) {
    if (controllerState & PAD_LEFT) {
        return SPRITE_DIRECTION_LEFT;
    }
    if (controllerState & PAD_UP) {
        return SPRITE_DIRECTION_UP;
    }
    if (controllerState & PAD_RIGHT) {
        return SPRITE_DIRECTION_RIGHT;
    }
    if (controllerState & PAD_DOWN) {
        return SPRITE_DIRECTION_DOWN;
    }
    return SPRITE_DIRECTION_STATIONARY;
}

static unsigned char is_horizontal_direction(unsigned char direction) {
    return direction == SPRITE_DIRECTION_LEFT || direction == SPRITE_DIRECTION_RIGHT;
}

static unsigned char is_vertical_direction(unsigned char direction) {
    return direction == SPRITE_DIRECTION_UP || direction == SPRITE_DIRECTION_DOWN;
}

static unsigned char is_opposite_direction(unsigned char lhs, unsigned char rhs) {
    return (lhs == SPRITE_DIRECTION_LEFT && rhs == SPRITE_DIRECTION_RIGHT) ||
           (lhs == SPRITE_DIRECTION_RIGHT && rhs == SPRITE_DIRECTION_LEFT) ||
           (lhs == SPRITE_DIRECTION_UP && rhs == SPRITE_DIRECTION_DOWN) ||
           (lhs == SPRITE_DIRECTION_DOWN && rhs == SPRITE_DIRECTION_UP);
}

static unsigned char can_snap_to_grid(int position) {
    unsigned char offset = (unsigned char)((position - PLAYER_GRID_ANCHOR_EXTENDED) & PLAYER_GRID_MASK_EXTENDED);
    return offset <= PLAYER_TURN_SNAP_THRESHOLD || offset >= (PLAYER_GRID_SIZE_EXTENDED - PLAYER_TURN_SNAP_THRESHOLD);
}

static int snap_to_grid(int position) {
    int aligned = position - PLAYER_GRID_ANCHOR_EXTENDED + (PLAYER_GRID_SIZE_EXTENDED >> 1);
    aligned &= ~PLAYER_GRID_MASK_EXTENDED;
    return aligned + PLAYER_GRID_ANCHOR_EXTENDED;
}

static unsigned char player_is_in_tunnel_row(int y_position) {
    unsigned char center_y = (unsigned char)((y_position >> PLAYER_POSITION_SHIFT) + NES_SPRITE_HEIGHT);
    unsigned char tile_row = center_y >> 3;
    return tile_row == (PACMAN_MAZE_ROW_OFFSET + PACMAN_TUNNEL_ROW);
}

static unsigned char player_maze_walkable_pixel(unsigned char x, unsigned char y) {
    unsigned char tile_row = y >> 3;
    unsigned char tile_col = x >> 3;

    if (tile_row < PACMAN_MAZE_ROW_OFFSET || tile_row >= (PACMAN_MAZE_ROW_OFFSET + PACMAN_MAZE_ROWS)) {
        return 0;
    }
    if (tile_row == (PACMAN_MAZE_ROW_OFFSET + PACMAN_TUNNEL_ROW) &&
        (tile_col < 2 || tile_col >= 21)) {
        return 1;
    }
    if (tile_col >= (PACMAN_MAZE_COL_OFFSET + PACMAN_MAZE_COLS)) {
        return 0;
    }

    return k_pacman_walkable_tiles[tile_row - PACMAN_MAZE_ROW_OFFSET][tile_col - PACMAN_MAZE_COL_OFFSET];
}

static unsigned char can_move_in_direction_from(unsigned char direction, int x_position, int y_position) {
    unsigned char center_x = (unsigned char)((x_position >> PLAYER_POSITION_SHIFT) + NES_SPRITE_WIDTH);
    unsigned char center_y = (unsigned char)((y_position >> PLAYER_POSITION_SHIFT) + NES_SPRITE_HEIGHT);

    switch (direction) {
        case SPRITE_DIRECTION_LEFT:
            if ((center_y >> 3) == (PACMAN_MAZE_ROW_OFFSET + PACMAN_TUNNEL_ROW) && center_x <= 16) {
                return 1;
            }
            return player_maze_walkable_pixel((unsigned char)(center_x - PLAYER_PROBE_LEFT_OFFSET), center_y);
        case SPRITE_DIRECTION_RIGHT:
            if ((center_y >> 3) == (PACMAN_MAZE_ROW_OFFSET + PACMAN_TUNNEL_ROW) && center_x >= (((PACMAN_MAZE_COLS - 1) << 3))) {
                return 1;
            }
            return player_maze_walkable_pixel((unsigned char)(center_x + PLAYER_PROBE_RIGHT_OFFSET), center_y);
        case SPRITE_DIRECTION_UP:
            return player_maze_walkable_pixel(center_x, (unsigned char)(center_y - PLAYER_PROBE_UP_OFFSET));
        case SPRITE_DIRECTION_DOWN:
            return player_maze_walkable_pixel(center_x, (unsigned char)(center_y + PLAYER_PROBE_DOWN_OFFSET));
        default:
            return 0;
    }
}

static void set_velocity_for_direction(unsigned char direction) {
    playerXVelocity = 0;
    playerYVelocity = 0;

    switch (direction) {
        case SPRITE_DIRECTION_LEFT:
            playerXVelocity = 0 - PLAYER_MOVE_SPEED;
            break;
        case SPRITE_DIRECTION_RIGHT:
            playerXVelocity = PLAYER_MOVE_SPEED;
            break;
        case SPRITE_DIRECTION_UP:
            playerYVelocity = 0 - PLAYER_MOVE_SPEED;
            break;
        case SPRITE_DIRECTION_DOWN:
            playerYVelocity = PLAYER_MOVE_SPEED;
            break;
    }
}

static unsigned char try_apply_queued_direction(unsigned char direction) {
    int snapped_position;

    if (is_horizontal_direction(direction)) {
        if (!can_snap_to_grid(playerYPosition)) {
            return 0;
        }

        snapped_position = snap_to_grid(playerYPosition);
        if (!can_move_in_direction_from(direction, playerXPosition, snapped_position)) {
            return 0;
        }

        playerYPosition = snapped_position;
        playerDirection = direction;
        return 1;
    }

    if (is_vertical_direction(direction)) {
        if (!can_snap_to_grid(playerXPosition)) {
            return 0;
        }

        snapped_position = snap_to_grid(playerXPosition);
        if (!can_move_in_direction_from(direction, snapped_position, playerYPosition)) {
            return 0;
        }

        playerXPosition = snapped_position;
        playerDirection = direction;
        return 1;
    }

    return 0;
}

// NOTE: This uses tempChar1 through tempChar3; the caller must not use these.
void update_player_sprite(void) {
    unsigned char directionIndex;
    unsigned char animationFrame;
    const unsigned char* spriteTiles;
    const unsigned char* spriteAttrs;

    // Calculate the position of the player itself, then use these variables to build it up with 4 8x8 NES sprites.
    rawXPosition = (playerXPosition >> PLAYER_POSITION_SHIFT);
    rawYPosition = (playerYPosition >> PLAYER_POSITION_SHIFT);
    directionIndex = get_pacman_direction_index();
    if (playerXVelocity != 0 || playerYVelocity != 0) {
        s_animation_frame = k_pacman_anim_sequence[frameCount & 0x07];
    } else if (s_animation_frame == 2) {
        s_animation_frame = 1;
    }
    animationFrame = s_animation_frame;

    if (animationFrame == 2) {
        spriteTiles = k_pacman_closed_sprite_tiles;
        spriteAttrs = k_pacman_closed_sprite_attrs;
    } else {
        spriteTiles = k_pacman_sprite_tiles[directionIndex][animationFrame];
        spriteAttrs = k_pacman_sprite_attrs[directionIndex][animationFrame];
    }

    // Clamp the player's sprite X Position to 0 to make sure we don't loop over, even if technically we have.
    if (rawXPosition > (SCREEN_EDGE_RIGHT + 4)) {
        rawXPosition = 0;
    }

    /* Slots 0..0x0C are unused during gameplay; force-hide them to avoid stale sprite fragments. */
    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0x00, 0x00, 0x00);
    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0x00, 0x00, 0x04);
    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0x00, 0x00, 0x08);
    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0x00, 0x00, 0x0C);
    
    if (playerInvulnerabilityTime && frameCount & PLAYER_INVULNERABILITY_BLINK_MASK) {
        // If the player is invulnerable, we hide their sprite about half the time to do a flicker animation.
        oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0x00, 0x00, PLAYER_SPRITE_INDEX);
        oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0x00, 0x00, PLAYER_SPRITE_INDEX+4);
        oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0x00, 0x00, PLAYER_SPRITE_INDEX+8);
        oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0x00, 0x00, PLAYER_SPRITE_INDEX+12);

    } else {
        oam_spr(rawXPosition, rawYPosition,
            spriteTiles[0],
            spriteAttrs[0],
            PLAYER_SPRITE_INDEX);
        oam_spr(rawXPosition + NES_SPRITE_WIDTH, rawYPosition,
            spriteTiles[1],
            spriteAttrs[1],
            PLAYER_SPRITE_INDEX+4);
        oam_spr(rawXPosition, rawYPosition + NES_SPRITE_HEIGHT,
            spriteTiles[2],
            spriteAttrs[2],
            PLAYER_SPRITE_INDEX+8);
        oam_spr(rawXPosition + NES_SPRITE_WIDTH, rawYPosition + NES_SPRITE_HEIGHT,
            spriteTiles[3],
            spriteAttrs[3],
            PLAYER_SPRITE_INDEX+12);
    }

}

void prepare_player_movement(void) {
    unsigned char requestedDirection;
    unsigned char movementDirection;

    lastControllerState = controllerState;
    controllerState = pad_poll(0);

    // If Start is pressed now, and was not pressed before...
    if (controllerState & PAD_START && !(lastControllerState & PAD_START)) {
        gameState = GAME_STATE_PAUSED;
        return;
    }
    if (playerControlsLockTime) {
        // If your controls are locked, just tick down the timer until they stop being locked. Don't read player input.
        playerControlsLockTime--;
    } else {
        requestedDirection = get_requested_direction();
        if (requestedDirection != SPRITE_DIRECTION_STATIONARY) {
            s_queued_direction = requestedDirection;
        }

        if (s_queued_direction != SPRITE_DIRECTION_STATIONARY) {
            if (is_opposite_direction(s_queued_direction, playerDirection) &&
                can_move_in_direction_from(s_queued_direction, playerXPosition, playerYPosition)) {
                playerDirection = s_queued_direction;
            } else {
                try_apply_queued_direction(s_queued_direction);
            }
        }

        if (is_horizontal_direction(playerDirection) && can_snap_to_grid(playerYPosition)) {
            playerYPosition = snap_to_grid(playerYPosition);
        } else if (is_vertical_direction(playerDirection) && can_snap_to_grid(playerXPosition)) {
            playerXPosition = snap_to_grid(playerXPosition);
        }

        movementDirection = playerDirection;

        if (movementDirection != SPRITE_DIRECTION_STATIONARY &&
            can_move_in_direction_from(movementDirection, playerXPosition, playerYPosition)) {
            set_velocity_for_direction(movementDirection);
        } else {
            playerXVelocity = 0;
            playerYVelocity = 0;

            if (s_queued_direction != SPRITE_DIRECTION_STATIONARY &&
                s_queued_direction != movementDirection &&
                try_apply_queued_direction(s_queued_direction) &&
                can_move_in_direction_from(playerDirection, playerXPosition, playerYPosition)) {
                set_velocity_for_direction(playerDirection);
            }
        }
    }

    // While we're at it, tick down the invulnerability timer if needed
    if (playerInvulnerabilityTime) {
        playerInvulnerabilityTime--;
    }

    nextPlayerXPosition = playerXPosition + playerXVelocity;
    nextPlayerYPosition = playerYPosition + playerYVelocity;

}

void do_player_movement(void) {

    // This will knock out the player's speed if they hit anything.
    test_player_tile_collision();
    // If the new player position hit any sprites, we'll find that out and knock it out here.
    handle_player_sprite_collision();

    playerXPosition = nextPlayerXPosition;
    playerYPosition = nextPlayerYPosition;

    if (player_is_in_tunnel_row(playerYPosition)) {
        if (playerXPosition < 0) {
            playerXPosition = PACMAN_TUNNEL_RIGHT_WRAP_X;
        } else if (playerXPosition > PACMAN_TUNNEL_RIGHT_WRAP_X) {
            playerXPosition = 0;
        }
    }

    rawXPosition = (playerXPosition >> PLAYER_POSITION_SHIFT);
    rawYPosition = (playerYPosition >> PLAYER_POSITION_SHIFT);
    /* Pac-Man: clamp to single screen, except for the wrap tunnel row. */
    if (!player_is_in_tunnel_row(playerYPosition)) {
        if (rawXPosition > SCREEN_EDGE_RIGHT || rawXPosition > (SCREEN_EDGE_RIGHT+4)) {
            playerXPosition = ((int)SCREEN_EDGE_RIGHT << PLAYER_POSITION_SHIFT);
            playerXVelocity = 0;
        } else if (rawXPosition < SCREEN_EDGE_LEFT) {
            playerXPosition = ((int)SCREEN_EDGE_LEFT << PLAYER_POSITION_SHIFT);
            playerXVelocity = 0;
        }
    }
    if (rawYPosition > SCREEN_EDGE_BOTTOM - 8) {
        playerYPosition = ((int)(SCREEN_EDGE_BOTTOM - 8) << PLAYER_POSITION_SHIFT);
        playerYVelocity = 0;
    } else if (rawYPosition < 6) {
        playerYPosition = (6 << PLAYER_POSITION_SHIFT);
        playerYVelocity = 0;
    }
}

void test_player_tile_collision(void) {
    if ((playerXVelocity != 0 || playerYVelocity != 0) &&
        !can_move_in_direction_from(playerDirection, playerXPosition, playerYPosition)) {
        playerXVelocity = 0;
        playerYVelocity = 0;
        nextPlayerXPosition = playerXPosition;
        nextPlayerYPosition = playerYPosition;
    }
}

void handle_player_sprite_collision(void) {
    // We store the last sprite hit when we update the sprites in `map_sprites.c`, so here all we have to do is react to it.
    if (lastPlayerSpriteCollisionId != NO_SPRITE_HIT) {
        currentMapSpriteIndex = lastPlayerSpriteCollisionId<<MAP_SPRITE_DATA_SHIFT;
        switch (currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE]) {
            case SPRITE_TYPE_HEALTH:
                // This if statement ensures that we don't remove hearts if you don't need them yet.
                if (playerHealth < playerMaxHealth) {
                    playerHealth += currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_HEALTH];
                    if (playerHealth > playerMaxHealth) {
                        playerHealth = playerMaxHealth;
                    }
                    // Hide the sprite now that it has been taken.
                    currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE] = SPRITE_TYPE_OFFSCREEN;

                    // Play the heart sound!
                    sfx_play(SFX_HEART, SFX_CHANNEL_3);

                    // Mark the sprite as collected, so we can't get it again.
                    currentMapSpritePersistance[playerOverworldPosition] |= bitToByte[lastPlayerSpriteCollisionId];
                }
                break;
            case SPRITE_TYPE_KEY:
                if (playerKeyCount < MAX_KEY_COUNT) {
                    playerKeyCount++;
                    currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE] = SPRITE_TYPE_OFFSCREEN;

                    sfx_play(SFX_KEY, SFX_CHANNEL_3);

                    // Mark the sprite as collected, so we can't get it again.
                    currentMapSpritePersistance[playerOverworldPosition] |= bitToByte[lastPlayerSpriteCollisionId];
                }
                break;
            case SPRITE_TYPE_REGULAR_ENEMY:
            case SPRITE_TYPE_INVULNERABLE_ENEMY:

                if (playerInvulnerabilityTime) {
                    return;
                }
                playerHealth -= currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_DAMAGE]; 
                // Since playerHealth is unsigned, we need to check for wraparound damage. 
                // NOTE: If something manages to do more than 16 damage at once, this might fail.
                if (playerHealth == 0 || playerHealth > 240) {
                    gameState = GAME_STATE_GAME_OVER;
                    music_stop();
                    sfx_play(SFX_GAMEOVER, SFX_CHANNEL_1);
                    return;
                }
                // Knock the player back
                playerControlsLockTime = PLAYER_DAMAGE_CONTROL_LOCK_TIME;
                playerInvulnerabilityTime = PLAYER_DAMAGE_INVULNERABILITY_TIME;
                if (playerDirection == SPRITE_DIRECTION_LEFT) {
                    // Punt them back in the opposite direction
                    playerXVelocity = PLAYER_MAX_VELOCITY;
                    // Reverse their velocity in the other direction, too.
                    playerYVelocity = 0 - playerYVelocity;
                } else if (playerDirection == SPRITE_DIRECTION_RIGHT) {
                    playerXVelocity = 0-PLAYER_MAX_VELOCITY;
                    playerYVelocity = 0 - playerYVelocity;
                } else if (playerDirection == SPRITE_DIRECTION_DOWN) {
                    playerYVelocity = 0-PLAYER_MAX_VELOCITY;
                    playerXVelocity = 0 - playerXVelocity;
                } else { // Make being thrown downward into a catch-all, in case your direction isn't set or something.
                    playerYVelocity = PLAYER_MAX_VELOCITY;
                    playerXVelocity = 0 - playerXVelocity;
                }
                sfx_play(SFX_HURT, SFX_CHANNEL_2);

                
                break;
            case SPRITE_TYPE_DOOR: 
                // Doors without locks are very simple - they just open! Hide the sprite until the user comes back...
                // note that we intentionally *don't* store this state, so it comes back next time.
                currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE] = SPRITE_TYPE_OFFSCREEN;
                break;
            case SPRITE_TYPE_LOCKED_DOOR:
                // First off, do you have a key? If so, let's just make this go away...
                if (playerKeyCount > 0) {
                    playerKeyCount--;
                    currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE] = SPRITE_TYPE_OFFSCREEN;

                    // Mark the door as gone, so it doesn't come back.
                    currentMapSpritePersistance[playerOverworldPosition] |= bitToByte[lastPlayerSpriteCollisionId];

                    break;
                }
                // So you don't have a key...
                // Okay, we collided with a door before we calculated the player's movement.
                // So, let's cancel it out and move on.
                playerXVelocity = 0;
                playerYVelocity = 0;
                playerControlsLockTime = 0;

                break;
            case SPRITE_TYPE_ENDGAME:
                gameState = GAME_STATE_CREDITS;
                break;
            case SPRITE_TYPE_NPC:
                // Okay, we collided with this NPC before we calculated the player's movement. After being moved, does the 
                // new player position also collide? If so, stop it. Else, let it go.

                // Calculate position...
                tempSpriteCollisionX = ((currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_X]) + ((currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_X + 1]) << 8));
                tempSpriteCollisionY = ((currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_Y]) + ((currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_Y + 1]) << 8));

                if (controllerState & PAD_A && !(lastControllerState & PAD_A)) {
                    // Show the text for the player on the first screen
                    if (playerOverworldPosition == 0) {
                        trigger_game_text(introductionText);
                    } else {
                        // If it's on another screen, show some different text :)
                        trigger_game_text(movedText);
                    }
                }
                break;


        }

    }
}
