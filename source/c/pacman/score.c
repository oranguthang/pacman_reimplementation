#include "source/c/pacman/score.h"

#include "source/c/library/bank_helpers.h"

CODE_BANK(PRG_BANK_PACMAN_SCORE);

static const unsigned char k_zero_score_digits[PACMAN_SCORE_DIGIT_COUNT] = {
    0, 0, 0, 0, 0, 0
};

static const unsigned char k_default_hi_score_digits[PACMAN_SCORE_DIGIT_COUNT] = {
    0, 1, 0, 0, 0, 0
};

unsigned char pacmanScoreDigits[PACMAN_SCORE_DIGIT_COUNT];
unsigned char pacmanHiScoreDigits[PACMAN_SCORE_DIGIT_COUNT];
unsigned char pacmanScoreDirty;
unsigned char pacmanHiScoreDirty;

static void copy_digits(unsigned char* dst, const unsigned char* src) {
    unsigned char index;

    for (index = 0; index < PACMAN_SCORE_DIGIT_COUNT; ++index) {
        dst[index] = src[index];
    }
}

static unsigned char score_is_greater(const unsigned char* lhs, const unsigned char* rhs) {
    unsigned char index;

    for (index = 0; index < PACMAN_SCORE_DIGIT_COUNT; ++index) {
        if (lhs[index] > rhs[index]) {
            return 1;
        }
        if (lhs[index] < rhs[index]) {
            return 0;
        }
    }

    return 0;
}

static void score_add_tens(unsigned char* digits, unsigned char tens) {
    unsigned char index = PACMAN_SCORE_DIGIT_COUNT - 2;
    unsigned char sum = digits[index] + tens;

    while (1) {
        if (sum < 10) {
            digits[index] = sum;
            return;
        }

        digits[index] = sum - 10;
        if (index == 0) {
            return;
        }

        --index;
        sum = digits[index] + 1;
    }
}

static void score_add_digits(unsigned char* digits, unsigned int points) {
    unsigned char add_digits[PACMAN_SCORE_DIGIT_COUNT];
    unsigned char index;
    unsigned char carry;
    unsigned char sum;

    if (points == 10u) {
        score_add_tens(digits, 1);
        return;
    }

    if (points == 50u) {
        score_add_tens(digits, 5);
        return;
    }

    for (index = 0; index < PACMAN_SCORE_DIGIT_COUNT; ++index) {
        add_digits[index] = 0;
    }

    while (points >= 10000u) {
        ++add_digits[1];
        points -= 10000u;
    }
    while (points >= 1000u) {
        ++add_digits[2];
        points -= 1000u;
    }
    while (points >= 100u) {
        ++add_digits[3];
        points -= 100u;
    }
    while (points >= 10u) {
        ++add_digits[4];
        points -= 10u;
    }
    add_digits[5] = (unsigned char)points;

    carry = 0;
    for (index = PACMAN_SCORE_DIGIT_COUNT; index != 0; ) {
        --index;
        sum = digits[index] + add_digits[index] + carry;
        if (sum >= 10) {
            digits[index] = sum - 10;
            carry = 1;
        } else {
            digits[index] = sum;
            carry = 0;
        }
    }
}

void pacman_score_reset(void) {
    copy_digits(pacmanScoreDigits, k_zero_score_digits);
    copy_digits(pacmanHiScoreDigits, k_default_hi_score_digits);
    pacmanScoreDirty = 1;
    pacmanHiScoreDirty = 1;
}

void pacman_score_add(unsigned int points) {
    score_add_digits(pacmanScoreDigits, points);
    if (score_is_greater(pacmanScoreDigits, pacmanHiScoreDigits)) {
        copy_digits(pacmanHiScoreDigits, pacmanScoreDigits);
        pacmanHiScoreDirty = 1;
    }
    pacmanScoreDirty = 1;
}
