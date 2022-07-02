#pragma once
#include <map>
#include "board.h"

enum Death_Type {ALIVE = 0, COLLISION = 1, SPACE = 2, HEALTH = 3};

class Move_Eval {
    public:
        Board* board;
        float you_eval;
        float you_space_eval;
        std::map<std::string, float> other_snake_evals;
        uint8_t you_depth;
        uint8_t you_space_depth;
        std::map<std::string, uint8_t> other_snake_depths;
        uint8_t you_death_type;

        Move_Eval(const Move_Eval& move_eval) : board(move_eval.board), you_eval(move_eval.you_eval), you_space_eval(move_eval.you_space_eval), other_snake_evals(move_eval.other_snake_evals),
            you_depth(move_eval.you_depth), other_snake_depths(move_eval.other_snake_depths), you_death_type(move_eval.you_death_type) {}
        Move_Eval(Board* board, float you_eval, float you_space_eval, std::map<std::string, float> other_snake_evals, uint8_t you_depth, std::map<std::string, uint8_t> other_snake_depths, uint8_t you_death_type = 0) :
            board(board), you_eval(you_eval), you_space_eval(you_space_eval), other_snake_evals(other_snake_evals), you_depth(you_depth), other_snake_depths(other_snake_depths), you_death_type(you_death_type) {}
};

class Max_Min_Queue_Item {
    public:
        std::vector<std::vector<Move_Eval*>* >* max_min_child_list;
        Board* parent;

        Max_Min_Queue_Item(std::vector<std::vector<Move_Eval*>* >* max_min_child_list, Board* parent) :
            max_min_child_list(max_min_child_list), parent(parent) {}

        ~Max_Min_Queue_Item() {delete max_min_child_list;}
};