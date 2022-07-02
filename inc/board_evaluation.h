#pragma once
#include "alpha_beta.h"
#include "board.h"
#include "snake.h"
#include "move_evaluation_classes.h"
#include "flood_fill.h"
#include <map>
#include <vector>
#include <string>

class Board_Evaluation {
    private:
        Move_Eval* move_eval;
        Board_Value_Modifiers* board_value_modifiers;

    public:
        Board_Evaluation(Move_Eval* move_eval, Board_Value_Modifiers* board_value_modifiers) : move_eval(move_eval), board_value_modifiers(board_value_modifiers) {}

        void evaluate_board();
        void evaluate_board_collisions();
        void evaluate_board_spacing();
        void update_depth();
};