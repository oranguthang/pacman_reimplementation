/*
main.c is the entrypoint of your game. Everything starts from here.
This has the main loop for the game, which is then used to call out to other code.
*/

#include "source/c/neslib.h"
#include "source/c/library/bank_helpers.h"
#include "source/c/configuration/game_states.h"
#include "source/c/menus/title.h"
#include "source/c/globals.h"
#include "source/c/menus/error.h"
#include "source/c/menus/credits.h"
#include "source/c/map/load_map.h"
#include "source/c/map/map.h"
#include "source/c/pacman/pellets.h"
#include "source/c/pacman/ghosts.h"
#include "source/c/pacman/score.h"
#include "source/c/pacman/demo.h"
#include "source/c/graphics/game_text.h"
#include "source/c/graphics/hud.h"
#include "source/c/graphics/fade_animation.h"
#include "source/c/sprites/player.h"
#include "source/c/menus/pause.h"
#include "source/c/sprites/map_sprites.h"
#include "source/c/sprites/sprite_definitions.h"
#include "source/c/menus/input_helpers.h"
#include "source/c/menus/game_over.h"


// Method to set a bunch of variables to default values when the system starts up.
// Note that if variables aren't set in this method, they will start at 0 on NES startup.
void initialize_variables(void) {

    playerOverworldPosition = 0; // Which tile on the overworld to start with; 0-62
    playerHealth = 5; // Player's starting health - how many hearts to show on the HUD.
    playerMaxHealth = 5; // Player's max health - how many hearts to let the player collect before it doesn't count.
    /* Pac-Man: spawn in the lower centre corridor, aligned to an empty maze cell. */
    playerXPosition = (84 << PLAYER_POSITION_SHIFT);
    playerYPosition = (172 << PLAYER_POSITION_SHIFT);
    playerXVelocity = 0;
    playerYVelocity = 0;
    playerDirection = SPRITE_DIRECTION_DOWN; // What direction to have the player face to start.

    lastPlayerSpriteCollisionId = NO_SPRITE_HIT;

    currentWorldId = WORLD_OVERWORLD; // The ID of the world to load.
    
    // Little bit of generic initialization below this point - we need to set
    // The system up to use a different hardware bank for sprites vs backgrounds.
    bank_spr(1);
}

void main(void) {
    fade_out_instant();
    gameState = GAME_STATE_SYSTEM_INIT;

    while (1) {
        unsigned char do_demo_vblank = 0;
        everyOtherCycle = !everyOtherCycle;
        switch (gameState) {
            case GAME_STATE_SYSTEM_INIT:
                initialize_variables();
                gameState = GAME_STATE_TITLE_DRAW;
                break;

            case GAME_STATE_TITLE_DRAW:
                banked_call(PRG_BANK_TITLE, draw_title_screen);
                music_play(SONG_TITLE);
                fade_in();
                break;
            case GAME_STATE_TITLE_INPUT:
                banked_call(PRG_BANK_TITLE, handle_title_input);
                break;
            case GAME_STATE_DEMO:
                bank_push(PRG_BANK_TITLE);
                demo_update(pad_poll(0));
                if (demo_should_exit_to_title() != 0) {
                    bank_pop();
                    gameState = GAME_STATE_TITLE_DRAW;
                    break;
                }
                if (demo_chase_transition_pending() != 0) {
                    demo_commit_chase_transition();
                }
                bank_pop();
                do_demo_vblank = 1;
                break;
            case GAME_STATE_POST_TITLE:

                music_stop();
                fade_out();

                banked_call(PRG_BANK_PACMAN_SCORE, pacman_score_reset);
                banked_call(PRG_BANK_MAP_LOGIC, pacman_pellets_init);
                banked_call(PRG_BANK_MAP_LOGIC, draw_current_map_to_a);
                banked_call(PRG_BANK_MAP_LOGIC, init_map);
                banked_call(PRG_BANK_HUD, draw_hud);
                banked_call(PRG_BANK_MAP_LOGIC, load_sprites);
                banked_call(PRG_BANK_PACMAN_GHOSTS, pacman_ghosts_init);

                // Seed the random number generator here, using the time since console power on as a seed
                set_rand(frameCount);
                
                // Map drawing is complete; let the player play the game!
                music_play(SONG_OVERWORLD);
                fade_in();
                gameState = GAME_STATE_RUNNING;
                break;

            case GAME_STATE_RUNNING:
            {
                unsigned char post_eat_freeze_active;
                // TODO: Might be nice to have this only called when we have something to update, and maybe only update the piece we 
                // care about. (For example, if you get a key, update the key count; not everything!
                banked_call(PRG_BANK_MAP_LOGIC, pacman_pellets_poll);
                banked_call(PRG_BANK_HUD, update_hud);
                bank_push(PRG_BANK_PACMAN_GHOSTS);
                post_eat_freeze_active = pacman_ghosts_is_post_eat_freeze_active();
                bank_pop();
                if (!post_eat_freeze_active) {
                    banked_call(PRG_BANK_PLAYER_SPRITE, prepare_player_movement);
                    banked_call(PRG_BANK_PLAYER_SPRITE, do_player_movement);
                    banked_call(PRG_BANK_MAP_LOGIC, pacman_pellets_try_eat);
                }
                banked_call(PRG_BANK_PACMAN_GHOSTS, pacman_ghosts_update);

                banked_call(PRG_BANK_PLAYER_SPRITE, update_player_sprite);
                break;
            }
            case GAME_STATE_SCREEN_SCROLL:
                // Pac-Man: no screen scrolling. Clamp player to screen bounds and resume.
                if (playerXPosition < (SCREEN_EDGE_LEFT << PLAYER_POSITION_SHIFT))
                    playerXPosition = (SCREEN_EDGE_LEFT << PLAYER_POSITION_SHIFT);
                if (playerXPosition > (SCREEN_EDGE_RIGHT << PLAYER_POSITION_SHIFT))
                    playerXPosition = (SCREEN_EDGE_RIGHT << PLAYER_POSITION_SHIFT);
                if (playerYPosition < (SCREEN_EDGE_TOP << PLAYER_POSITION_SHIFT))
                    playerYPosition = (SCREEN_EDGE_TOP << PLAYER_POSITION_SHIFT);
                if (playerYPosition > (SCREEN_EDGE_BOTTOM << PLAYER_POSITION_SHIFT))
                    playerYPosition = (SCREEN_EDGE_BOTTOM << PLAYER_POSITION_SHIFT);
                playerXVelocity = 0;
                playerYVelocity = 0;
                gameState = GAME_STATE_RUNNING;
                break;
            case GAME_STATE_SHOWING_TEXT:
                banked_call(PRG_BANK_GAME_TEXT, draw_game_text);
                gameState = GAME_STATE_RUNNING;
                break;
            case GAME_STATE_PAUSED:
                banked_call(PRG_BANK_HUD, show_hud_pause);
                banked_call(PRG_BANK_HUD, update_hud);
                banked_call(PRG_BANK_PAUSE_MENU, handle_pause_input);
                banked_call(PRG_BANK_HUD, hide_hud_pause);
                break;
            case GAME_STATE_GAME_OVER:
                fade_out();

                // Draw the "you lose" screen
                banked_call(PRG_BANK_GAME_OVER, draw_game_over_screen);
                fade_in();
                banked_call(PRG_BANK_MENU_INPUT_HELPERS, wait_for_start);
                fade_out();
                reset();
                break;
            case GAME_STATE_CREDITS:
                music_stop();
                sfx_play(SFX_WIN, SFX_CHANNEL_1);

                fade_out();
                // Draw the "you won" screen
                banked_call(PRG_BANK_CREDITS_MENU, draw_win_screen);
                fade_in();
                banked_call(PRG_BANK_MENU_INPUT_HELPERS, wait_for_start);
                fade_out();

                // Folow it up with the credits.
                banked_call(PRG_BANK_CREDITS_MENU, draw_credits_screen);
                fade_in();
                banked_call(PRG_BANK_MENU_INPUT_HELPERS, wait_for_start);
                fade_out();
                reset();
                break;
            default:
                crash_error_use_banked_details(ERR_UNKNOWN_GAME_STATE, ERR_UNKNOWN_GAME_STATE_EXPLANATION, "gameState value", gameState);
                
        }
        if (do_demo_vblank != 0 && gameState == GAME_STATE_DEMO) {
            ppu_wait_nmi();
            banked_call(PRG_BANK_TITLE, demo_vblank);
            continue;
        }

        ppu_wait_nmi();
    }
}
