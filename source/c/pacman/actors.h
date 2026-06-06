#ifndef PACMAN_DEMO_ACTORS_H
#define PACMAN_DEMO_ACTORS_H

void actors_reset(void);
void actors_update_positions(void);
void actors_build_oam(void);
void actors_flush_oam(void);
void actors_rotate_runners(void);

void actor_set_vel_x(unsigned char actor, unsigned char hi, unsigned char lo);
void actor_set_vel_y(unsigned char actor, unsigned char hi, unsigned char lo);
unsigned char actor_get_vel_x_hi(unsigned char actor);
void actor_set_spr_pos(unsigned char actor, unsigned char x_hi, unsigned char x_lo, unsigned char y_hi, unsigned char y_lo);
unsigned char actor_get_spr_x(unsigned char actor);
unsigned char actor_get_spr_y(unsigned char actor);
void actor_set_type(unsigned char actor, unsigned char type);
void actor_set_attr(unsigned char actor, unsigned char attr);
unsigned char actor_get_type(unsigned char actor);
unsigned char actor_get_attr(unsigned char actor);

#endif
