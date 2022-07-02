#pragma once
#include <map>
#include <vector>
#include <array>
#include <queue>
#include <string>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <stdlib.h>
#include <chrono>
#include "board.h"
#include "move_evaluation_classes.h"
#include "../api/json.hpp"

enum starting_behaviours {ControlLarge = 0, EatLarge = 1, Small = 2, ControlSmall = 3, EatSmall = 4};

class Board_Value_Modifiers {
    public:
        float win_value = 1.00f;
        float tie_value = -0.499999f;
        float suicide_value = -0.999999f;
        float predicted_health_death = -0.999998f;
        float loss_value = -1.00f;

        float total_occupied_ratio = 0.0f;
        float space_total_modifier = 1.0f;
        float food_control_modifier = 1.0f;
        float eat_control_modifier = 1.0f;
        float dominated_control_modifier = 1.0f;
        float space_to_food_modifier = 0.0f;
        float food_control_size = 0.4f;
        float opposing_snake_death_size = 0.75f;
        uint8_t food_control_size_offset = 2;
        uint8_t food_control_size_range = 3;
        uint8_t health_cutoff_for_modifier = 25;
        uint8_t food_value = 3;
        uint8_t starting_behaviour = 0;
        
        uint16_t smallest_snake_len = 0;
        std::map<std::string, uint16_t> original_len;

        std::string game_mode = "normal";
        std::string game_map = "standard";
        int hazard_damage = 0;
        uint8_t shrink_N_Turns = 0;
        

        Board_Value_Modifiers(nlohmann::json*);

};

class AlphaBeta {
    private:
        Board* main_board;
        Board* chosen_board;

        Board_Value_Modifiers* board_value_modifiers;

        float max_time_micro = 0.300f;//seconds
        uint8_t max_depth = 32;
        uint8_t max_depth_hit = 0;

        uint8_t debug_limit = 1;
        uint8_t debug_child_counter = 0;

        //tuple of board, depth, alpha, beta
        std::queue<Board*>* evaluate_queue;
        //queue of list of list of move evaluations
        std::vector<Max_Min_Queue_Item*>* max_min_queue;
        //dict of string of board as key and value as item
        std::map<std::string, Move_Eval* >* value_dict;

        //queue of list of list of move evaluations that are to be deleted
        std::vector<Move_Eval*>* delete_move_eval_queue;
        //queue of list of list of move evaluations that are to be deleted
        std::vector<std::vector<Move_Eval*>* >* delete_max_min_child_queue;
        //queue of list of list of move evaluations that are to be deleted
        std::vector<Max_Min_Queue_Item*>* delete_max_min_queue;

        //used when have all death moves
        float highest_board_value = -1.0;

        float large_tile_modifier = 4.0f;
        float medium_tile_modifier = 3.5f;
        float small_tile_modifier = 2.5f;

        clock_t start_time;
        clock_t end_time;
        float time_difference;

        std::array<uint16_t, 33> nodes_evaluated;

    public:
        AlphaBeta(nlohmann::json);
        ~AlphaBeta();

        void operator()();

        void breadth_first_search_loop();
        void board_eval_bfs(Board*);
        void max_min(Max_Min_Queue_Item*);
        Move_Eval* find_min_board(std::vector<Move_Eval*>*);
        Move_Eval* find_max_board(std::vector<Move_Eval*>*);
        std::string get_move_from_board();

        Board* get_chosen_board() {return chosen_board;}
        Board* get_main_board() {return main_board;}
        uint8_t get_max_depth_hit() {return max_depth_hit;}
        float get_max_time_micro() {return max_time_micro;}
        void print_nodes_evaluated();
};