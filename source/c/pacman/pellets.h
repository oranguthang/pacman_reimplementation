#ifndef PACMAN_PELLETS_H
#define PACMAN_PELLETS_H

#define PACMAN_PELLET_NONE 0
#define PACMAN_PELLET_REGULAR 1
#define PACMAN_PELLET_POWER 2

extern unsigned char pacmanPelletsRemaining;

void pacman_pellets_init(void);
void pacman_pellets_poll(void);
unsigned char pacman_pellets_adjust_maze_tile(unsigned char row, unsigned char col, unsigned char tile_id);
void pacman_pellets_try_eat(void);

#endif
