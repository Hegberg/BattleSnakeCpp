#pragma once
#include "alpha_beta.h"
#include "board.h"
#include "snake.h"
#include "move_evaluation_classes.h"
#include <map>
#include <vector>
#include <map>
#include <array>
#include <string>
#include <cmath>

class Board_Spacing {
    private:
        Move_Eval* move_eval;
        Board_Value_Modifiers* board_value_modifiers;

    public:
        Board_Spacing(Move_Eval*, Board_Value_Modifiers*);
        ~Board_Spacing();

        void determine_board_spacing();
        void calculate_board_values();
};