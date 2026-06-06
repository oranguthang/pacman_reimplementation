#ifndef PACMAN_DEMO_H
#define PACMAN_DEMO_H

void demo_init(void);
void demo_update(unsigned char pad);
void demo_vblank(void);
void demo_commit_chase_transition(void);
unsigned char demo_chase_transition_pending(void);
unsigned char demo_should_exit_to_title(void);

#endif
