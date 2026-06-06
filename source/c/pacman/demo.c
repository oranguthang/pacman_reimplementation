#include "source/c/pacman/demo.h"

#include "source/c/pacman/actors.h"
#include "source/c/pacman/compat.h"
#include "source/c/pacman/render.h"
#include "source/c/library/bank_helpers.h"
#include "source/c/menus/title.h"

CODE_BANK(PRG_BANK_TITLE);

enum {
    DEMO_PHASE_NAMES = 0,
    DEMO_PHASE_CHASE = 1
};

enum {
    CHASE_SUBSTATE_SETUP = 0,
    CHASE_SUBSTATE_RUN_FROM = 2,
    CHASE_SUBSTATE_RUN_TOWARD = 4
};

static const unsigned char k_spawn_checkpoints[4] = {0xE0, 0xD1, 0xC2, 0xB3};
static const unsigned char k_ghost_score_tiles[4] = {0x2D, 0x2F, 0x32, 0x34};
static const unsigned char k_chase_anim_lut[20] = {
    0x04,0x04,0x04,0x05,0x05,0x04,0x01,0x01,0x0C,0x0D,
    0x08,0x08,0x08,0x09,0x09,0x08,0x01,0x01,0x1E,0x1F
};

static unsigned char demo_phase;
static unsigned char demo_timer;
static unsigned char demo_packet_idx;
static unsigned char input_prev_pad;
static unsigned char chase_substate;
static unsigned char chase_ghost_count;
static unsigned char chase_eat_timer;
static unsigned char chase_frame;
static unsigned char chase_anim_cnt;
static unsigned char demo_pending_packet;
static unsigned char demo_exit_to_title;
static unsigned char demo_begin_chase_pending;
static unsigned char demo_pending_ghost_strip;
static unsigned char demo_oam_buf[64];
static unsigned char demo_oam_dirty;

static void chase_update_anim(void) {
    unsigned char bank = (actor_get_vel_x_hi(0) & 0x80) != 0 ? 0x00 : 0x0A;
    unsigned char idx;
    unsigned char i;

    ++chase_anim_cnt;
    idx = (unsigned char)((chase_anim_cnt & 0x07) + bank);
    actor_set_type(0, k_chase_anim_lut[idx]);

    if ((chase_frame & 0x07) != 0) {
        return;
    }

    idx = (unsigned char)(((chase_frame & 0x08) != 0 ? 1 : 0) + bank + 8);
    for (i = 1; i <= 4; ++i) {
        if (actor_get_type(i) != 0) {
            actor_set_type(i, k_chase_anim_lut[idx]);
        }
    }
}

static void chase_handle_contact(void) {
    unsigned char pac_x;
    unsigned char threshold;
    unsigned char slot;

    if ((actor_get_vel_x_hi(0) & 0x80) != 0) {
        return;
    }

    pac_x = actor_get_spr_x(0);
    threshold = (unsigned char)(pac_x + 8);

    for (slot = 1; slot <= 4; ++slot) {
        unsigned char sx = actor_get_spr_x(slot);
        if (sx != 0 && sx < threshold) {
            unsigned char score_idx = chase_ghost_count;
            if (score_idx > 3) {
                score_idx = 3;
            }
            ++chase_ghost_count;
            chase_eat_timer = 0x40;
            actor_set_type(slot, k_ghost_score_tiles[score_idx]);
            actor_set_attr(slot, 0x00);
            actor_set_type(0, 0x00);
            actor_set_spr_pos(0, pac_x, 0x00, 0x00, 0x00);
            return;
        }
    }
}

static void chase_update_frame(void) {
    actors_update_positions();
    actors_rotate_runners();
    chase_update_anim();
    chase_handle_contact();
    actors_build_oam();
}

static void chase_setup(void) {
    chase_substate = CHASE_SUBSTATE_RUN_FROM;
    chase_ghost_count = 0;
    chase_eat_timer = 0;
    chase_frame = 0;
    chase_anim_cnt = 0;
    actors_reset();

    actor_set_spr_pos(0, 0xF4, 0x00, 0xA8, 0x00);
    actor_set_vel_x(0, 0xFF, 0x00);
    actor_set_vel_y(0, 0x00, 0x00);
    actor_set_type(0, 0x01);
    actor_set_attr(0, 0x00);
    actors_build_oam();
}

static void chase_spawn_ghost(void) {
    unsigned char slot;

    for (slot = 1; slot <= 4; ++slot) {
        if (actor_get_spr_x(slot) == 0) {
            actor_set_spr_pos(slot, 0xF4, 0x00, 0xA8, 0x00);
            actor_set_vel_x(slot, 0xFF, 0x00);
            actor_set_vel_y(slot, 0x00, 0x00);
            actor_set_type(slot, 0x0C);
            actor_set_attr(slot, chase_ghost_count);
            ++chase_ghost_count;
            actors_build_oam();
            return;
        }
    }
}

static void chase_run_from(unsigned char* exit_to_title) {
    unsigned char pac_x;
    unsigned char i;

    (void)exit_to_title;
    chase_update_frame();
    pac_x = actor_get_spr_x(0);

    for (i = 0; i < 4; ++i) {
        if (pac_x == k_spawn_checkpoints[i]) {
            chase_spawn_ghost();
        }
    }

    if (pac_x == 0x40) {
        chase_substate = CHASE_SUBSTATE_RUN_TOWARD;
        chase_ghost_count = 0;
        actor_set_vel_x(0, 0x01, 0x50);
        for (i = 1; i <= 4; ++i) {
            actor_set_vel_x(i, 0x00, 0xC0);
            actor_set_attr(i, 0x01);
        }
        actors_build_oam();
    }
}

static void chase_run_toward(unsigned char* exit_to_title) {
    if (chase_eat_timer != 0) {
        actors_rotate_runners();
        --chase_eat_timer;
        if (chase_eat_timer == 0) {
            unsigned char slot;
            for (slot = 1; slot <= 4; ++slot) {
                if ((actor_get_type(slot) & 0xE0) != 0) {
                    unsigned char sx = actor_get_spr_x(slot);
                    actor_set_type(slot, 0x00);
                    actor_set_attr(slot, 0x00);
                    actor_set_spr_pos(slot, 0x00, 0x00, 0x00, 0x00);
                    actor_set_vel_x(slot, 0x00, 0x00);
                    actor_set_vel_y(slot, 0x00, 0x00);
                    actor_set_spr_pos(0, sx, 0x00, 0xA8, 0x00);
                    break;
                }
            }
            if (chase_ghost_count >= 4) {
                *exit_to_title = 1;
            }
        }
        return;
    }

    chase_update_frame();
}

static void demo_begin_chase_phase(void) {
    demo_begin_chase_pending = 1;
}

void demo_commit_chase_transition(void) {
    if (demo_begin_chase_pending == 0) {
        return;
    }

    chase_setup();
    render_chase_init();
    demo_phase = DEMO_PHASE_CHASE;
    demo_timer = 0;
    demo_begin_chase_pending = 0;
}

void demo_init(void) {
    unsigned char i;

    demo_phase = DEMO_PHASE_NAMES;
    demo_timer = 0;
    demo_packet_idx = 0;
    /* Absorb the current controller state on title->demo handoff so a held or
       freshly sampled START does not immediately bounce attract mode back out. */
    input_prev_pad = pad_poll(0);
    chase_substate = CHASE_SUBSTATE_SETUP;
    chase_ghost_count = 0;
    chase_eat_timer = 0;
    chase_frame = 0;
    chase_anim_cnt = 0;
    demo_pending_packet = 0xFF;
    demo_pending_ghost_strip = 0xFF;
    demo_oam_dirty = 0;
    demo_exit_to_title = 0;
    demo_begin_chase_pending = 0;

    for (i = 0; i < 64; i += 4) {
        demo_oam_buf[i] = 0xFF;
        demo_oam_buf[(unsigned char)(i + 1)] = 0x00;
        demo_oam_buf[(unsigned char)(i + 2)] = 0x00;
        demo_oam_buf[(unsigned char)(i + 3)] = 0x00;
    }

    render_demo_names_screen();
}

void demo_vblank(void) {
    if (demo_phase == DEMO_PHASE_NAMES) {
        if (demo_pending_packet != 0xFF) {
            render_demo_names_packet_vblank(demo_pending_packet);
            if (demo_pending_packet == 1 || demo_pending_packet == 3 ||
                demo_pending_packet == 5 || demo_pending_packet == 7) {
                demo_pending_ghost_strip = (unsigned char)((demo_pending_packet - 1) >> 1);
                demo_oam_dirty = 1;
            }
            demo_pending_packet = 0xFF;
        } else if (demo_pending_ghost_strip != 0xFF) {
            render_demo_names_ghost_vblank(demo_pending_ghost_strip, demo_oam_buf);
            demo_pending_ghost_strip = 0xFF;
            demo_oam_dirty = 1;
        } else if (demo_oam_dirty != 0) {
            render_demo_names_oam_flush(demo_oam_buf);
            demo_oam_dirty = 0;
        }
        render_demo_scroll(0);
    } else {
        actors_flush_oam();
        render_demo_scroll(0);
    }
}

void demo_update(unsigned char pad) {
    unsigned char start_edge = (unsigned char)(((pad & PAD_START) != 0) && ((input_prev_pad & PAD_START) == 0));

    if (start_edge) {
        demo_exit_to_title = 1;
    } else if (demo_phase == DEMO_PHASE_NAMES) {
        ++demo_timer;
        if (demo_packet_idx < 9) {
            if (demo_timer >= 0x30) {
                unsigned char pkt;
                demo_timer = 0;
                ++demo_packet_idx;
                pkt = demo_packet_idx;
                demo_pending_packet = pkt;
                if (pkt == 1 || pkt == 3 || pkt == 5 || pkt == 7) {
                    demo_pending_ghost_strip = (unsigned char)((pkt - 1) >> 1);
                    demo_oam_dirty = 1;
                }
            }
        } else if (demo_timer >= 0x80) {
            demo_begin_chase_phase();
        }
    } else {
        ++demo_timer;
        ++chase_frame;

        if (chase_substate == CHASE_SUBSTATE_RUN_FROM) {
            chase_run_from(&demo_exit_to_title);
        } else {
            chase_run_toward(&demo_exit_to_title);
        }
    }

    input_prev_pad = pad;
}

unsigned char demo_should_exit_to_title(void) {
    return demo_exit_to_title;
}

unsigned char demo_chase_transition_pending(void) {
    return demo_begin_chase_pending;
}
