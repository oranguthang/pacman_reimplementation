#ifndef PACMAN_GHOSTS_H
#define PACMAN_GHOSTS_H

#define PRG_BANK_PACMAN_GHOSTS 1

void pacman_ghosts_init(void);
void pacman_ghosts_update(void);
void pacman_ghosts_start_frightened(void);
unsigned char pacman_ghosts_is_post_eat_freeze_active(void);

#endif
