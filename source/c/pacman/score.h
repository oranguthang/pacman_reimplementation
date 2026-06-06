#ifndef PACMAN_SCORE_H
#define PACMAN_SCORE_H

#define PRG_BANK_PACMAN_SCORE 1

#define PACMAN_SCORE_DIGIT_COUNT 6

extern unsigned char pacmanScoreDigits[PACMAN_SCORE_DIGIT_COUNT];
extern unsigned char pacmanHiScoreDigits[PACMAN_SCORE_DIGIT_COUNT];
extern unsigned char pacmanScoreDirty;
extern unsigned char pacmanHiScoreDirty;

void pacman_score_reset(void);
void pacman_score_add(unsigned int points);

#endif
