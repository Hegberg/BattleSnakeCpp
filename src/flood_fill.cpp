#include "../inc/flood_fill.h"

Board_Spacing::Board_Spacing(Move_Eval* move_eval, Board_Value_Modifiers* board_value_modifiers) {
    this->move_eval = move_eval;
    this->board_value_modifiers = board_value_modifiers;
}

Board_Spacing::~Board_Spacing() {

}

//Add algorithm here to determine the amount of space a snake has for any particular board
void Board_Spacing::determine_board_spacing() {
    
}

//Add algorithm here to determine how an amount of space translates to a value between 1 (win) and -1 (loss)
void Board_Spacing::calculate_board_values() {

    this->move_eval->you_eval = 0.1234f;

    for (uint16_t i = 0; i < move_eval->board->get_other_snakes()->size(); ++i) {
        this->move_eval->other_snake_evals[move_eval->board->get_other_snakes()->at(i)->get_id()] = 0.1234f;
    }
}