#include "../inc/board_evaluation.h"

void Board_Evaluation::evaluate_board() {
    //fill out our snake values
    this->move_eval->you_eval = 0.0f;
    this->move_eval->you_space_eval = 0.0f;
    this->move_eval->you_depth = 0U;
    this->move_eval->you_space_depth = 0U;
    this->move_eval->you_death_type = ALIVE;

    //fill out other snake values
    for (uint16_t i = 0; i < this->move_eval->board->get_other_snakes()->size(); ++i) {
        this->move_eval->other_snake_evals[(*this->move_eval->board->get_other_snakes())[i]->get_id()] = 0.0f;
        this->move_eval->other_snake_depths[(*this->move_eval->board->get_other_snakes())[i]->get_id()] = 0U;
    }

    //fill out dead snake values
    for (uint16_t i = 0; i < this->move_eval->board->get_dead_snakes()->size(); ++i) {
        this->move_eval->other_snake_evals[(*this->move_eval->board->get_dead_snakes())[i]->get_id()] = this->board_value_modifiers->loss_value;
        this->move_eval->other_snake_depths[(*this->move_eval->board->get_dead_snakes())[i]->get_id()] = 0U;
    }

    //check for collisions
    this->evaluate_board_collisions();

    //If colliding and dieing, set death type to collision
    if (this->move_eval->you_eval == this->board_value_modifiers->tie_value || this->move_eval->you_eval == this->board_value_modifiers->suicide_value || this->move_eval->you_eval == this->board_value_modifiers->loss_value) {
        this->move_eval->you_death_type = COLLISION;
    }

    //check spacing value
    this->evaluate_board_spacing();

    //If dieing to space, set death type to space
    if (this->move_eval->you_death_type == ALIVE && this->move_eval->you_eval == this->board_value_modifiers->loss_value) {
        this->move_eval->you_death_type = SPACE;
    }
    this->update_depth();

    return;
}

void Board_Evaluation::evaluate_board_collisions() {

    std::map<std::string, std::vector<Snake*> > collision_placements;
    std::map<std::string, Snake* > dead_collision_placements;

    uint16_t amount_of_snakes = 0;
    
    //get all collision placement for dead snakes
    //with tile as key and list of snakes as snakes hitting tile
    for (uint16_t i = 0; i < this->move_eval->board->get_dead_snakes()->size(); ++i) {
        std::string tile = std::to_string((*(*this->move_eval->board->get_dead_snakes())[i]->get_head())['x']);
        tile.append("_");
        tile.append(std::to_string((*(*this->move_eval->board->get_dead_snakes())[i]->get_head())['y']));
        //if key does not exist, init
        if (dead_collision_placements.find(tile) == dead_collision_placements.end()) {
            dead_collision_placements[tile] = (*this->move_eval->board->get_dead_snakes())[i];
        }
        //otherwise check which is largest dead snake to occupy
        else if (dead_collision_placements.find(tile) != dead_collision_placements.end() && dead_collision_placements[tile]->get_length() < (*this->move_eval->board->get_dead_snakes())[i]->get_length()) {
            dead_collision_placements[tile] = (*this->move_eval->board->get_dead_snakes())[i];
        }
        ++amount_of_snakes;
    }

    //for my snake if alive, get placement
    if (this->move_eval->board->get_you() != 0) {
        std::string tile = std::to_string((*this->move_eval->board->get_you()->get_head())['x']);
        tile.append("_");
        tile.append(std::to_string((*this->move_eval->board->get_you()->get_head())['y']));
        //if key does not exist, init list
        if (collision_placements.find(tile) == collision_placements.end()) {
            collision_placements[tile] = {};
        }
        collision_placements[tile].push_back(this->move_eval->board->get_you());
        ++amount_of_snakes;
    }

    //get all collision placement for other live snakes
    //with tile as key and list of snakes as snakes hitting tile
    for (uint16_t i = 0; i < this->move_eval->board->get_other_snakes()->size(); ++i) {
        std::string tile = std::to_string((*(*this->move_eval->board->get_other_snakes())[i]->get_head())['x']);
        tile.append("_");
        tile.append(std::to_string((*(*this->move_eval->board->get_other_snakes())[i]->get_head())['y']));
        //if key does not exist, init list
        if (collision_placements.find(tile) == collision_placements.end()) {
            collision_placements[tile] = {};
        }
        collision_placements[tile].push_back((*this->move_eval->board->get_other_snakes())[i]);
        ++amount_of_snakes;
    }

    //check if multiple snakes exist for collision space
    //if so determine outcomes and value
    std::map<std::string, std::vector<Snake*> >::iterator col_it = collision_placements.begin();

    while(col_it != collision_placements.end()) {
        //if more than 1 snake on tile, there is collision
        if (col_it->second.size() > 1 || (col_it->second.size() == 1 && dead_collision_placements.find(col_it->first) != dead_collision_placements.end())) {
            //find longest snake size and if multiple of same longest len
            uint16_t longest_snake_size = col_it->second[0]->get_length();
            bool multiple_large_snakes = false;
            for (uint16_t i = 1; i < col_it->second.size(); ++i) {
                if (col_it->second[i]->get_length() > longest_snake_size) {
                    longest_snake_size = col_it->second[i]->get_length();
                    multiple_large_snakes = false;
                } else if (col_it->second[i]->get_length() == longest_snake_size) {
                    multiple_large_snakes = true;
                }
            }

            //check to see if dead snake larger or as large as others colliding on tile
            if (dead_collision_placements.find(col_it->first) != dead_collision_placements.end() && dead_collision_placements[col_it->first]->get_length() > longest_snake_size) {
                longest_snake_size = dead_collision_placements[col_it->first]->get_length();
                multiple_large_snakes = false;
            } else if (dead_collision_placements.find(col_it->first) != dead_collision_placements.end() && dead_collision_placements[col_it->first]->get_length() == longest_snake_size) {
                multiple_large_snakes = true;
            }

            //go through snakes, assign values based off size compared to longest size
            for (uint16_t i = 0; i < col_it->second.size(); ++i) {
                
                //if biggest and not a tie, win
                if (col_it->second[i]->get_length() == longest_snake_size && !multiple_large_snakes) {
                    //if me
                    if (col_it->second[i]->get_id() == this->move_eval->board->get_you()->get_id()) {
                        this->move_eval->you_eval = this->board_value_modifiers->win_value;
                    }
                    //if not me
                    else {
                        this->move_eval->other_snake_evals[col_it->second[i]->get_id()] = this->board_value_modifiers->win_value;
                    }
                }
                //if biggest and tie and only 2 snakes on board, tie
                else if (col_it->second[i]->get_length() == longest_snake_size && multiple_large_snakes && (amount_of_snakes == 2 || amount_of_snakes == col_it->second.size())) {
                    //if me
                    if (col_it->second[i]->get_id() == this->move_eval->board->get_you()->get_id()) {
                        this->move_eval->you_eval = this->board_value_modifiers->tie_value;
                    }
                    //if not me
                    else {
                        this->move_eval->other_snake_evals[col_it->second[i]->get_id()] = this->board_value_modifiers->tie_value;
                    }
                }
                //if biggest and tie and more than 2 snakes on board, suicide
                else if (col_it->second[i]->get_length() == longest_snake_size && multiple_large_snakes && amount_of_snakes > 2) {
                    //if me
                    if (col_it->second[i]->get_id() == this->move_eval->board->get_you()->get_id()) {
                        this->move_eval->you_eval = this->board_value_modifiers->suicide_value;
                    }
                    //if not me
                    else {
                        this->move_eval->other_snake_evals[col_it->second[i]->get_id()] = this->board_value_modifiers->suicide_value;
                    }
                }
                //if not biggest snake, loss
                else if (col_it->second[i]->get_length() < longest_snake_size) {
                    //if me
                    if (col_it->second[i]->get_id() == this->move_eval->board->get_you()->get_id()) {
                        this->move_eval->you_eval = this->board_value_modifiers->loss_value;
                    }
                    //if not me
                    else {
                        this->move_eval->other_snake_evals[col_it->second[i]->get_id()] = this->board_value_modifiers->loss_value;
                    }
                }
            }
        }

        ++col_it;
    }
}

void Board_Evaluation::evaluate_board_spacing() {
    
    Board_Spacing board_spacing = Board_Spacing(this->move_eval, this->board_value_modifiers);
    board_spacing.determine_board_spacing();
    board_spacing.calculate_board_values();

}

void Board_Evaluation::update_depth() {
    this->move_eval->you_depth += this->move_eval->board->get_depth();
    for (uint16_t i = 0; i < this->move_eval->board->get_other_snakes()->size(); ++i) {
        this->move_eval->other_snake_depths[(*this->move_eval->board->get_other_snakes())[i]->get_id()] += this->move_eval->board->get_depth();
    }
}