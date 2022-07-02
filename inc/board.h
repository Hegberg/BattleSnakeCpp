#pragma once
#include <map>
#include <vector>
#include <string>
#include <cstdint>
#include <iostream>
#include <stdlib.h>
#include <cmath>
#include <algorithm>
#include "../api/json.hpp"
#include "snake.h"

class Board {
    private:
        int height;
        int width;
        int turn;
        std::vector<Snake*>* other_snakes;
        std::vector<Snake*>* dead_snakes;
        std::vector<std::string>* dead_next_round_snakes;
        Snake* you;
        std::map<std::string, bool>* occupied_tiles;
        std::vector<Board*>* children;
        std::map<std::string, std::map<char, int>* >* food;
        std::map<std::string, std::map<char, int>* >* hazards;

        uint8_t depth;
        float alpha;
        float beta;

        uint8_t hazard_damage;
        std::string game_mode;
        std::string game_map;

    public:
        Board(nlohmann::json*, nlohmann::json*, int, std::string, std::string);
        Board(Board*);
        ~Board();
        
        int get_height() {return height;}
        int get_width() {return width;}
        int get_turn() {return turn;}
        std::vector<Snake*>* get_other_snakes() {return other_snakes;}
        std::vector<Snake*>* get_dead_snakes() {return dead_snakes;}
        std::map<std::string, bool>* get_occupied_tiles() {return occupied_tiles;}
        Snake* get_you() {return you;}
        std::map<std::string, std::map<char, int>* >* get_food() {return food;}
        std::map<std::string, std::map<char, int>* >* get_hazards() {return hazards;}
        std::vector<Board*>* get_children() {return children;}

        void add_dead_next_round_snake(std::string snake_id) {dead_next_round_snakes->push_back(snake_id);}
        std::vector<std::string>* get_dead_next_round_snakes() {return dead_next_round_snakes;}
        
        uint8_t get_depth() {return depth;}
        float get_alpha() {return alpha;}
        float get_beta() {return beta;}
        std::string get_game_mode() {return game_mode;}
        std::string get_game_map() {return game_map;}
        void set_depth(uint8_t);
        void set_alpha(float);
        void set_beta(float);
        void set_hazard_damage(int hazard_damage) {this->hazard_damage = hazard_damage;}
        int get_hazard_damage() {return hazard_damage;};

        std::string get_move_from_new_board(Board* new_board);
        void create_child_board_states();
        void create_occupied_tiles();
        void remove_snake_at_index(int);
        void find_food_being_eaten(std::vector<std::map<char,int> >&, Snake*);
        std::string board_to_string();
        void print_board();
        
        bool is_food_at_location(std::string);
        bool is_hazard_at_location(std::string);
};

std::vector<std::pair<int,int> > get_possible_moves(Board* board, Snake* snake);

bool check_health(Board* board, Snake* snake, int x, int y);

bool sort_by_sec(const std::tuple<Snake*, int, int> &a, const std::tuple<Snake*, int, int> &b);