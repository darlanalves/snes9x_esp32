#pragma once

#include <stdint.h>
#include <stdbool.h>

enum {
    BUTTON_A = 0,
    BUTTON_B,
    BUTTON_X,
    BUTTON_Y,

    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_UP,
    BUTTON_DOWN,

    BUTTON_ST,
    BUTTON_SEL,

    BUTTON_L,
    BUTTON_R,

    BUTTON_SW,   

    // last entry
    NUM_BTNS,
    BUTTON_ANY
};

void ctrl_init();
uint32_t ctrl_button_state_update();
bool ctrl_is_pressed(int button);
