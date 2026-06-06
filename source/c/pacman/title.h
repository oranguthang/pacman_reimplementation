#ifndef PACMAN_TITLE_H
#define PACMAN_TITLE_H

void title_init(void);

/*
 * Call once per frame before ppu_wait_nmi().
 * Returns: 0 = still on title, 1 = start game, 2 = go to demo.
 * Sets *players_2p to 1 if 2-player mode selected.
 */
unsigned char title_update(unsigned char* players_2p);

#endif
