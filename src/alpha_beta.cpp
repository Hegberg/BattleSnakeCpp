#include "../inc/alpha_beta.h"
#include "../inc/move_evaluation_classes.h"
#include "../inc/board_evaluation.h"
#include <tuple>

//Uncomment define if want debug information printed on failed tests
//#define DEBUG

AlphaBeta::AlphaBeta(nlohmann::json data) {
    this->start_time = clock();
    
    this->board_value_modifiers = new Board_Value_Modifiers(&data["game"]);
    
    this->main_board = new Board(&data["board"], &data["you"], data["turn"], this->board_value_modifiers->game_mode, this->board_value_modifiers->game_map);
    this->main_board->set_hazard_damage(this->board_value_modifiers->hazard_damage);
    this->chosen_board = 0;

    //if large board, limit to 1 depth
    if (this->main_board->get_width() > 22) {
        this->max_depth = 1;
    }

    this->evaluate_queue = new std::queue<Board*>;
    this->max_min_queue = new std::vector<Max_Min_Queue_Item*>;
    this->value_dict = new std::map<std::string, Move_Eval* >;
    this->delete_move_eval_queue = new std::vector<Move_Eval*>;
    this->delete_max_min_child_queue = new std::vector<std::vector<Move_Eval*>* >;
    this->delete_max_min_queue = new std::vector<Max_Min_Queue_Item*>;

    bool multiple_large_snakes = false;
    //set largest and smallest snakes to my value, then check against other snakes in main board
    uint16_t largest_snake_len = main_board->get_you()->get_length();
    uint16_t second_largest_snake_len = 0;
    this->board_value_modifiers->smallest_snake_len = main_board->get_you()->get_length();
    uint16_t self_snake_len = main_board->get_you()->get_length();

    this->board_value_modifiers->original_len[main_board->get_you()->get_id()] = main_board->get_you()->get_length();

    for (uint8_t i = 0; i < main_board->get_other_snakes()->size(); ++i) {
        if ((*main_board->get_other_snakes())[i]->get_length() < largest_snake_len && (*main_board->get_other_snakes())[i]->get_length() > second_largest_snake_len) {
            second_largest_snake_len = (*main_board->get_other_snakes())[i]->get_length();
        }
        if ((*main_board->get_other_snakes())[i]->get_length() < this->board_value_modifiers->smallest_snake_len) {
            this->board_value_modifiers->smallest_snake_len = (*main_board->get_other_snakes())[i]->get_length();
        }
        if ((*main_board->get_other_snakes())[i]->get_length() > largest_snake_len) {
            second_largest_snake_len = largest_snake_len;
            largest_snake_len = (*main_board->get_other_snakes())[i]->get_length();
            multiple_large_snakes = false;
        } else if ((*main_board->get_other_snakes())[i]->get_length() == largest_snake_len) {
            second_largest_snake_len = largest_snake_len;
            multiple_large_snakes = true;
        }

        this->board_value_modifiers->original_len[(*main_board->get_other_snakes())[i]->get_id()] = (*main_board->get_other_snakes())[i]->get_length();
    }

    float scale_top = 0.4f;
    float scale_bottom = 0.2f;

    //scale behaviours based on size of snakes, < 20% to > 40%
    float board_used_percentage = ((float)this->main_board->get_occupied_tiles()->size()) / ((float)this->main_board->get_height() * (float)this->main_board->get_width());
    float snake_tile_modifier = 1.0f;
    if (board_used_percentage <= scale_bottom) {
        board_used_percentage = 0.5f;
    } else if (board_used_percentage >= scale_top) {
        board_used_percentage = 1.0f;
    } else {
        board_used_percentage = board_used_percentage - scale_bottom; //scale score of bottom to 0.0f, now between 0.0 and top - bottom, so 0.0 - 0.2
        board_used_percentage = board_used_percentage * 2.5f;//scale between 0.0 and 0.5
        board_used_percentage = board_used_percentage + 0.5f;//add 0.5 to scale between 0.5 and 1.0
    }
    large_tile_modifier = large_tile_modifier * board_used_percentage;
    medium_tile_modifier = medium_tile_modifier * board_used_percentage;
    small_tile_modifier = small_tile_modifier * board_used_percentage;


    float occupied_tile_modifier = 1.0;
    //if tied for largest, value eating
    if (largest_snake_len == self_snake_len && multiple_large_snakes) {
        occupied_tile_modifier = this->medium_tile_modifier;
        this->board_value_modifiers->starting_behaviour = EatLarge;
    //if largest snake and no ties, favour controling area over eating
    } else if (self_snake_len == largest_snake_len) {
        occupied_tile_modifier = this->large_tile_modifier;
        this->board_value_modifiers->starting_behaviour = ControlLarge;
    //if small, mark as small for later eval
    } else if (largest_snake_len != self_snake_len) {
        occupied_tile_modifier = this->small_tile_modifier;
        this->board_value_modifiers->starting_behaviour = Small;
    }

    int board_space = (this->main_board->get_height() * this->main_board->get_width()) - this->main_board->get_hazards()->size();
    this->board_value_modifiers->total_occupied_ratio = (this->main_board->get_occupied_tiles()->size() * occupied_tile_modifier) / (float)board_space;
    if (this->board_value_modifiers->total_occupied_ratio > 1.0f) {this->board_value_modifiers->total_occupied_ratio = 1.0f;}
    this->board_value_modifiers->space_total_modifier = 1.2f * this->board_value_modifiers->total_occupied_ratio;//-0.55 to 0.55 if full board
    this->board_value_modifiers->food_control_modifier = 0.4f * this->board_value_modifiers->total_occupied_ratio;//-0.25 to 0.25 if full board
    this->board_value_modifiers->eat_control_modifier = 1.6f * this->board_value_modifiers->total_occupied_ratio;//-0.8 to 0.8 if empty board
    this->board_value_modifiers->dominated_control_modifier = 0.2f;//-0.1 to 0.1 any board

    //if controlling large area since big AND map isn't arcade maze, deprioritize food
    if (this->board_value_modifiers->starting_behaviour == ControlLarge && this->board_value_modifiers->game_map != "arcade_maze") {
        float multiplier = (float)largest_snake_len - (float)second_largest_snake_len;
        if (multiplier > this->board_value_modifiers->food_control_size_range + this->board_value_modifiers->food_control_size_offset) {
            multiplier = (float)this->board_value_modifiers->food_control_size_range + (float)this->board_value_modifiers->food_control_size_offset;
        } else if (multiplier < this->board_value_modifiers->food_control_size_offset) {
            multiplier = (float)this->board_value_modifiers->food_control_size_offset;
        }
        multiplier = multiplier - (float)this->board_value_modifiers->food_control_size_offset;
        //make modifier between control_range but with a size offset for when to start caring less for controlling food
        this->board_value_modifiers->space_to_food_modifier = (0.0f + (multiplier / (float)this->board_value_modifiers->food_control_size_range));//0.0 to 1.0, takes space to 2.0 and food to 0 if full
    }

    for (uint16_t i = 0; i < this->nodes_evaluated.size(); ++i) {
        this->nodes_evaluated[i] = 0;
    }
}

AlphaBeta::~AlphaBeta() {
    //deletes original board and all the boards spanning from it
    delete this->main_board;
    //delete this->chosen_board;

    for (uint16_t i = 0; i < this->delete_move_eval_queue->size(); ++i) {
        delete (*this->delete_move_eval_queue)[i];
    }
    delete this->delete_move_eval_queue;

    for (uint16_t i = 0; i < this->delete_max_min_child_queue->size(); ++i) {
        delete (*this->delete_max_min_child_queue)[i];
    }
    delete this->delete_max_min_child_queue;

    for (uint16_t i = 0; i < this->delete_max_min_queue->size(); ++i) {
        delete (*this->delete_max_min_queue)[i];
    }
    delete this->delete_max_min_queue;

    //queues will be empty when deleting, so just delete queue
    delete this->evaluate_queue;
    //TODO properly delete this memory, this queue is empty but items inside were not deleted before
    delete this->max_min_queue;

    delete this->value_dict;

    delete this->board_value_modifiers;
}

Board_Value_Modifiers::Board_Value_Modifiers(nlohmann::json* data) {
    //if old version of data, fill in filler info for old tests so don't fail
    if (data->at("ruleset")["version"] == "v.1.2.3" || data->at("ruleset")["version"] == "Mojave/3.1.6" || data->at("ruleset")["version"] == "v1.0.17" || data->at("ruleset")["version"] == "v1.0.13" || data->at("ruleset")["version"] == "v1.0.15") {
        this->game_mode = "standard";
        this->hazard_damage = 14;
        this->shrink_N_Turns = 20;
    } else { //else parse data correctly
        this->game_mode = data->at("ruleset")["name"];
        this->game_map = data->at("map");
        this->hazard_damage = data->at("ruleset")["settings"]["hazardDamagePerTurn"];
        this->shrink_N_Turns = data->at("ruleset")["settings"]["royale"]["shrinkEveryNTurns"];
    }
}

void AlphaBeta::operator()() {
    this->breadth_first_search_loop();
}

void AlphaBeta::breadth_first_search_loop() {
    
    this->evaluate_queue->push(this->main_board);

    while (this->evaluate_queue->size() > 0) {
        board_eval_bfs(this->evaluate_queue->front());
        //do not delete board memory here, since still being used for rest of algorithm, and referenced as child of father
        this->evaluate_queue->pop();
    }
    
    while (this->max_min_queue->size() > 0) {
        max_min(this->max_min_queue->back());
        //do not delete board memory here, since still being used for rest of algorithm, and referenced as child of father
        this->max_min_queue->pop_back();
    }
}

void AlphaBeta::board_eval_bfs(Board* board) {
    //update farthest depth hit
    if (board->get_depth() > this->max_depth_hit) {
        this->max_depth_hit = board->get_depth();
    }

    this->end_time = clock();
    this->time_difference = float(this->end_time - this->start_time) / CLOCKS_PER_SEC;

    if (time_difference > this->max_time_micro || board->get_depth() >= this->max_depth) {
        return;
    }

    board->create_child_board_states();

    //id is string of snake, value is <board_copy, my_value, other_snake_values, my_depth, other_snake_depths>
    std::map<std::string, std::vector<Move_Eval*>* > move_evaluations;

    //go through children, evaluate each
    //if child does not include me, remove it from children
    //if no children, give board state -1 value
    for (uint16_t i = 0; i < board->get_children()->size(); ++i) {
        //grab my body on new board

        Snake* you = (*board->get_children())[i]->get_you();

        //check if snake not present if it died
        if (you == 0) {
            if (move_evaluations.find("no_snake") == move_evaluations.end()) {
                //not found
                std::vector<Move_Eval*>* new_move_eval_list = new std::vector<Move_Eval*>;
                move_evaluations["no_snake"] = new_move_eval_list;
            }
            Move_Eval* no_snake = new Move_Eval((*board->get_children())[i], -1.0, -1.0, {}, 0, {}, ALIVE);
            this->delete_move_eval_queue->push_back(no_snake);
            move_evaluations["no_snake"]->push_back(no_snake);
            continue;
        }

        //check if body already exists in move eval or not, and if not create
        if (move_evaluations.find((*board->get_children())[i]->get_you()->body_to_string()) == move_evaluations.end()) {
            std::vector<Move_Eval*>* new_move_eval_list = new std::vector<Move_Eval*>;
            move_evaluations[(*board->get_children())[i]->get_you()->body_to_string()] = new_move_eval_list;
        }
        
        //get evaluation for board, add eval info to move_eval
        Move_Eval* move_eval = new Move_Eval((*board->get_children())[i], 0.0, 0.0, {}, 0, {}, ALIVE);
        this->delete_move_eval_queue->push_back(move_eval);
        
        Board_Evaluation board_eval = Board_Evaluation(move_eval, this->board_value_modifiers);
        board_eval.evaluate_board();
        move_evaluations[(*board->get_children())[i]->get_you()->body_to_string()]->push_back(move_eval);

        //check not going over time
        this->end_time = clock();
        this->time_difference = float(this->end_time - this->start_time) / CLOCKS_PER_SEC;
        
        this->nodes_evaluated[move_eval->board->get_depth()] += 1;
        
        if (time_difference > this->max_time_micro || board->get_depth() >= this->max_depth) {
            break;
        }
    }
 
    this->time_difference = float(this->end_time - this->start_time) / CLOCKS_PER_SEC;

    //list of boards to be added to max_min_queue
    std::vector<std::vector<Move_Eval*>* >* max_min_child_list = new std::vector<std::vector<Move_Eval*>* >;

    std::map<std::string, std::vector<Move_Eval*>* >::iterator move_eval_it = move_evaluations.begin();
    uint16_t outer_loop_counter = 0;

    Move_Eval* lowest_evaluation = 0;
    float highest_low_eval = this->board_value_modifiers->loss_value;
    std::queue<std::pair<Move_Eval*, int> > death_queue;
    
    //go through all our moves
    while (move_eval_it != move_evaluations.end()) {

        std::vector<Move_Eval*>* max_min_child = new std::vector<Move_Eval*>;
        this->delete_max_min_child_queue->push_back(max_min_child);
        max_min_child_list->push_back(max_min_child);
        
        //go through array of possible opponent moves for our move
        for (uint16_t i = 0; i < move_eval_it->second->size(); ++i) {
            //if only my snake an started with more than 1 snake, win so put value as 1 as long as not -1
            if ((*move_eval_it->second)[i]->board->get_other_snakes()->size() == 0 && 
                this->main_board->get_other_snakes()->size() > 0 &&
                (*move_eval_it->second)[i]->you_eval != this->board_value_modifiers->loss_value) {
                (*move_eval_it->second)[i]->you_eval = this->board_value_modifiers->win_value;
            //else if only one opponent and his value -1, set my snake to 1 as long as not -1
            } else if ((*move_eval_it->second)[i]->other_snake_evals.size() == 1 && 
                (*move_eval_it->second)[i]->you_eval != this->board_value_modifiers->loss_value) {
                std::map<std::string, float>::iterator map_it = (*move_eval_it->second)[i]->other_snake_evals.begin();
                while(map_it != (*move_eval_it->second)[i]->other_snake_evals.end()) {
                    if (map_it->second == this->board_value_modifiers->loss_value) {
                        (*move_eval_it->second)[i]->you_eval = this->board_value_modifiers->win_value;
                    }
                    ++map_it;
                }
            }

            //if score is lowest record
            if ((lowest_evaluation == 0 || (*move_eval_it->second)[i]->you_eval < lowest_evaluation->you_eval)) {
                lowest_evaluation = (*move_eval_it->second)[i];
            }

            //if it is me with a win or loss value, don't even bother continuing line
            //so skip to next possible board
            if ((*move_eval_it->second)[i]->you_eval == this->board_value_modifiers->win_value ||
                (*move_eval_it->second)[i]->you_eval == this->board_value_modifiers->loss_value ||
                (*move_eval_it->second)[i]->you_eval == this->board_value_modifiers->suicide_value ||
                (*move_eval_it->second)[i]->you_eval == this->board_value_modifiers->predicted_health_death ||
                (*move_eval_it->second)[i]->you_eval == this->board_value_modifiers->tie_value) {
                
                //this->value_dict[move_eval_it->second[i]->board->board_to_string()] = move_eval_it->second[i];
                this->value_dict->insert({(*move_eval_it->second)[i]->board->board_to_string(), (*move_eval_it->second)[i]});
                (*max_min_child_list)[outer_loop_counter]->push_back((*move_eval_it->second)[i]);
                
                //of lowest value is a suicide or loss, this value is suicide or loss, and dieing to space not collision, add to queue (don't add collision since no way can go past that)
                if ((lowest_evaluation->you_eval == this->board_value_modifiers->loss_value || lowest_evaluation->you_eval == this->board_value_modifiers->suicide_value) 
                    && ((*move_eval_it->second)[i]->you_eval == this->board_value_modifiers->loss_value || (*move_eval_it->second)[i]->you_eval == this->board_value_modifiers->suicide_value)
                    && (*move_eval_it->second)[i]->you_death_type == SPACE) {
                    death_queue.push({(*move_eval_it->second)[i], outer_loop_counter});
                }
                continue;
            }

            //if snake has eval of -1, set it to be dead_snake in it's childs boards
            for (uint16_t j = 0; j < (*move_eval_it->second)[i]->board->get_other_snakes()->size(); ++j) {
                if ((*move_eval_it->second)[i]->other_snake_evals[(*(*move_eval_it->second)[i]->board->get_other_snakes())[j]->get_id()] == this->board_value_modifiers->loss_value
                    || (*move_eval_it->second)[i]->other_snake_evals[(*(*move_eval_it->second)[i]->board->get_other_snakes())[j]->get_id()] == this->board_value_modifiers->suicide_value) {
                    
                    (*move_eval_it->second)[i]->board->add_dead_next_round_snake((*(*move_eval_it->second)[i]->board->get_other_snakes())[j]->get_id());
                }
            }

            //add board to be added to max_min_queue
            max_min_child_list->at(max_min_child_list->size() - 1)->push_back((*move_eval_it->second)[i]);

            //add children to queue to evaluate at lower depths
            this->evaluate_queue->push((*move_eval_it->second)[i]->board);

        }

        //reset lowest since going through new move
        if (lowest_evaluation != 0) {
            if (highest_low_eval < lowest_evaluation->you_eval) {
                highest_low_eval = lowest_evaluation->you_eval;
            }
            lowest_evaluation = 0;
        }

        //increment iterator
        ++move_eval_it;

        ++outer_loop_counter;
    }

    //of my highest eval of min evals is a loss, start adding loss evals for further investigation
    if (highest_low_eval == this->board_value_modifiers->loss_value || highest_low_eval == this->board_value_modifiers->suicide_value) {
        while (death_queue.size() > 0) {
            this->evaluate_queue->push(death_queue.front().first->board);
            //remove boards value from dictionary so has clean sheet for next depth
            this->value_dict->erase(death_queue.front().first->board->board_to_string());
            //add board to be added to max_min_queue
            max_min_child_list->at(death_queue.front().second)->push_back(death_queue.front().first);
            death_queue.pop();
        }
    }

    //add boards to max_min_queue
    if (max_min_child_list->size() > 0 && max_min_child_list->at(0)->size() > 0) {
        Max_Min_Queue_Item* queue_item = new Max_Min_Queue_Item(max_min_child_list, board);
        this->delete_max_min_queue->push_back(queue_item);
        this->max_min_queue->push_back(queue_item);
    }

    //delete move_evaluations since out of scope
    std::map<std::string, std::vector<Move_Eval*>* >::iterator move_eval_delete_it = move_evaluations.begin();
    //go through all our moves, don't delete Move_Evals though
    //if Move_Evals created, need to exist to end of max min function
    while (move_eval_delete_it != move_evaluations.end()) {
        delete move_eval_delete_it->second;
        ++move_eval_delete_it;
    }
    
    this->time_difference = float(this->end_time - this->start_time) / CLOCKS_PER_SEC;

}

void AlphaBeta::max_min(Max_Min_Queue_Item* max_min_queue_item) {

    //if depth is farther than max fleshed out depth, return out
    //if max_depth_hit == the parent -1, then the child is == to max depth, and should be ignored
    
    if (this->max_depth_hit > 1 && this->max_depth_hit <= max_min_queue_item->parent->get_depth() + 1) {
        return;
    }

    //list of min values for each of my moves
    std::vector<Move_Eval*> min_child_move_list;

    //go through all my moves, find min value for each move
    for (uint16_t i = 0; i < max_min_queue_item->max_min_child_list->size(); ++i) {

        //if board has value from farther depth, take that value over current value
        for (uint16_t j = 0; j < (*max_min_queue_item->max_min_child_list)[i]->size(); ++j) {
            //if can find board already in dict, replace current values with dict values
            if (this->value_dict->find((*(*max_min_queue_item->max_min_child_list)[i])[j]->board->board_to_string()) != this->value_dict->end()) {

                (*(*max_min_queue_item->max_min_child_list)[i])[j]->you_eval = (*this->value_dict)[(*(*max_min_queue_item->max_min_child_list)[i])[j]->board->board_to_string()]->you_eval;

                (*(*max_min_queue_item->max_min_child_list)[i])[j]->you_space_eval = (*this->value_dict)[(*(*max_min_queue_item->max_min_child_list)[i])[j]->board->board_to_string()]->you_space_eval;

                (*(*max_min_queue_item->max_min_child_list)[i])[j]->you_depth = (*this->value_dict)[(*(*max_min_queue_item->max_min_child_list)[i])[j]->board->board_to_string()]->you_depth;

                (*(*max_min_queue_item->max_min_child_list)[i])[j]->you_death_type = (*this->value_dict)[(*(*max_min_queue_item->max_min_child_list)[i])[j]->board->board_to_string()]->you_death_type;

                (*(*max_min_queue_item->max_min_child_list)[i])[j]->other_snake_evals = (*this->value_dict)[(*(*max_min_queue_item->max_min_child_list)[i])[j]->board->board_to_string()]->other_snake_evals;

                (*(*max_min_queue_item->max_min_child_list)[i])[j]->other_snake_depths = (*this->value_dict)[(*(*max_min_queue_item->max_min_child_list)[i])[j]->board->board_to_string()]->other_snake_depths;
            }
        }

        //if for my move there is more than 0 boards, find board with min value
        //add the board to min list, and store value of board in value_dict
        if ((*max_min_queue_item->max_min_child_list)[i]->size() > 0) {
            min_child_move_list.push_back(find_min_board((*max_min_queue_item->max_min_child_list)[i]));
            this->value_dict->insert({min_child_move_list.back()->board->board_to_string(), min_child_move_list.back()});
        }
#ifdef DEBUG
        std::cout << "------\n";
        min_child_move_list.back()->board->print_board();
        std::cout << "Min Val: " << min_child_move_list.back()->you_eval << ' ' << std::to_string(min_child_move_list.back()->you_depth) << ' ' << min_child_move_list.back()->you_space_eval << " Death:" << std::to_string(min_child_move_list.back()->you_death_type) << "\n";
        std::cout << "------\n";
#endif
    }

    Move_Eval* max_eval;
    //if more than 0 min evals, find max of all the min
    if (min_child_move_list.size() > 0) {
        max_eval = find_max_board(&min_child_move_list);
        this->value_dict->insert({max_min_queue_item->parent->board_to_string(), max_eval});
#ifdef DEBUG
        std::cout << "------------------\n";
        std::cout << "Max Eval: " << max_eval->you_eval << ' ' << std::to_string(max_eval->you_depth) << ' ' << max_eval->you_space_eval << " Death:" << std::to_string(max_eval->you_death_type) << "\n";
        std::cout << "Parent: " << this->value_dict->at(max_min_queue_item->parent->board_to_string())->you_eval << ' ' << this->value_dict->at(max_min_queue_item->parent->board_to_string())->you_space_eval << '\n';
        std::cout << "Par: " << max_min_queue_item->parent->board_to_string() << '\n';
        std::cout << "----------------\n";
#endif 
        //if this is depth 0, set this board as chosen board
        if (max_min_queue_item->parent->get_depth() == 0) {
            this->chosen_board = max_eval->board;
        }
    }
}

//basically min but takes depth into account
Move_Eval* AlphaBeta::find_min_board(std::vector<Move_Eval*>* move_eval_list) {
    if (move_eval_list->size() == 0) {
        return 0;
    }
    uint16_t min_index = 0;
    for (uint16_t i = 1; i < move_eval_list->size(); ++i) {
        //smaller value
        if ((*move_eval_list)[i]->you_eval < (*move_eval_list)[min_index]->you_eval) {
            min_index = i;
        }
        //else if same value and I am not losing
        //and larger depth (too draw out game since not losing)
        else if ((*move_eval_list)[i]->you_eval == (*move_eval_list)[min_index]->you_eval && (*move_eval_list)[i]->you_eval > this->board_value_modifiers->loss_value
                && (*move_eval_list)[i]->you_depth > (*move_eval_list)[min_index]->you_depth) {
            min_index = i;
        }
        //else if same value and i'm losing, and depth is smaller to kill me faster
        else if ((*move_eval_list)[i]->you_eval == (*move_eval_list)[min_index]->you_eval && (*move_eval_list)[i]->you_eval == this->board_value_modifiers->loss_value
                && (*move_eval_list)[i]->you_depth < (*move_eval_list)[min_index]->you_depth) {
            min_index = i;
        }
        //else if same value and same depth, favour collision death over space (space gaurunteed, collision has chance to have enemy mess up)
        else if ((*move_eval_list)[i]->you_eval == (*move_eval_list)[min_index]->you_eval && (*move_eval_list)[i]->you_depth == (*move_eval_list)[min_index]->you_depth
                && ((*move_eval_list)[i]->you_death_type == COLLISION && ((*move_eval_list)[min_index]->you_death_type == SPACE
                || (*move_eval_list)[min_index]->you_death_type == HEALTH))) {
            min_index = i;
        }
        //else if same value and same depth, choose one with worse space value to win or die in better scenario
        else if ((*move_eval_list)[i]->you_eval == (*move_eval_list)[min_index]->you_eval && (*move_eval_list)[i]->you_depth == (*move_eval_list)[min_index]->you_depth
                && (*move_eval_list)[i]->you_death_type == (*move_eval_list)[min_index]->you_death_type && (*move_eval_list)[i]->you_space_eval < (*move_eval_list)[min_index]->you_space_eval) {
            min_index = i;
        }
    }
    return (*move_eval_list)[min_index];
}

//basically max but takes depth into account
Move_Eval* AlphaBeta::find_max_board(std::vector<Move_Eval*>* move_eval_list) {
    if (move_eval_list->size() == 0) {
        return 0;
    }
    uint16_t max_index = 0;
    for (uint16_t i = 1; i < move_eval_list->size(); ++i) {
        //larger value //and not suicide or loss
        if ((*move_eval_list)[i]->you_eval > (*move_eval_list)[max_index]->you_eval 
            /*&& move_eval_list->at(i)->you_eval != this->board_value_modifiers->loss_value && move_eval_list->at(i)->you_eval != this->board_value_modifiers->suicide_value*/) {
            max_index = i;
        }
        //else if same value and I not losing,
        //and smaller depth (too win fast)
        else if ((*move_eval_list)[i]->you_eval == (*move_eval_list)[max_index]->you_eval && (*move_eval_list)[i]->you_eval > this->board_value_modifiers->suicide_value
                && (*move_eval_list)[i]->you_depth < (*move_eval_list)[max_index]->you_depth) {
            max_index = i;
        }
        //else if same value and i'm losing, and depth is larger to kill me slower
        else if ((*move_eval_list)[i]->you_eval == (*move_eval_list)[max_index]->you_eval && ((*move_eval_list)[i]->you_eval == this->board_value_modifiers->loss_value
                || (*move_eval_list)[i]->you_eval == this->board_value_modifiers->suicide_value) && (*move_eval_list)[i]->you_depth > (*move_eval_list)[max_index]->you_depth) {
            max_index = i;
        }
        //else if both suicide/loss and depth on same level, choose suicide over loss
        else if ((*move_eval_list)[i]->you_eval == this->board_value_modifiers->suicide_value && (*move_eval_list)[max_index]->you_eval == this->board_value_modifiers->loss_value
                && (*move_eval_list)[i]->you_depth == (*move_eval_list)[max_index]->you_depth) {
            max_index = i;
        }
        //else if same value and same depth, favour collision death over space (space gaurunteed, collision has chance to have enemy mess up)
        else if ((*move_eval_list)[i]->you_eval == (*move_eval_list)[max_index]->you_eval && (*move_eval_list)[i]->you_depth == (*move_eval_list)[max_index]->you_depth
                && ((*move_eval_list)[i]->you_death_type == COLLISION && ((*move_eval_list)[max_index]->you_death_type == SPACE
                || (*move_eval_list)[max_index]->you_death_type == HEALTH))) {
                max_index = i;
        }
        //else if same value and same depth and same death type, choose one with better space value to win or die in better scenario
        else if ((*move_eval_list)[i]->you_eval == (*move_eval_list)[max_index]->you_eval && (*move_eval_list)[i]->you_depth == (*move_eval_list)[max_index]->you_depth
                && (*move_eval_list)[i]->you_death_type == (*move_eval_list)[max_index]->you_death_type && (*move_eval_list)[i]->you_space_eval > (*move_eval_list)[max_index]->you_space_eval) {
                max_index = i;
        }
    }
    return (*move_eval_list)[max_index];
}

std::string AlphaBeta::get_move_from_board() {
    //check to make sure there is a main board, or return random
    if (this->chosen_board == 0) {
        return "right";
    }
    Snake* self_pointer = 0; 
    //check if I am dead and multiple dead snakes
    if (this->chosen_board->get_you() == 0 && this->chosen_board->get_dead_snakes()->size() > 1) {
        return "right";
    }
    //check if I am dead and I am only dead snake
    else if (this->chosen_board->get_you() == 0 && this->chosen_board->get_dead_snakes()->size() == 1) {
        self_pointer = (*this->chosen_board->get_dead_snakes())[0];
    }
    //else I am alive 
    else {
        self_pointer = this->chosen_board->get_you();
    }

    //if don't exist, return random
    if (self_pointer == 0) {
        return "right";
    }

    if (this->board_value_modifiers->game_mode == "wrapped") {
        //at far right, want to loop to far left, go right
        if ((*self_pointer->get_head())['x'] == 0 && (*this->main_board->get_you()->get_head())['x'] == this->main_board->get_width() - 1) {
            return "right";
        }

        //at far left, want to loop to far right, go left
        if ((*self_pointer->get_head())['x'] == this->main_board->get_width() - 1 && (*this->main_board->get_you()->get_head())['x'] == 0) {
            return "left";
        }

        //at top, want to loop to bottom, go up
        if ((*self_pointer->get_head())['y'] == 0 && (*this->main_board->get_you()->get_head())['y'] == this->main_board->get_height() - 1) {
            return "up";
        }

        //at bottom, want to loop to top, go down
        if ((*self_pointer->get_head())['y'] == this->main_board->get_height() - 1 && (*this->main_board->get_you()->get_head())['y'] == 0) {
            return "down";
        }

    }

    //else go through where I am to was and return appropiate direction
    //to the right
    if ((*self_pointer->get_head())['x'] > (*this->main_board->get_you()->get_head())['x']) {
        return "right";
    }

    //to the left
    if ((*self_pointer->get_head())['x'] < (*this->main_board->get_you()->get_head())['x']) {
        return "left";
    }

    //to the top
    if ((*self_pointer->get_head())['y'] > (*this->main_board->get_you()->get_head())['y']) {
        return "up";
    }

    //to the bottom
    if ((*self_pointer->get_head())['y'] < (*this->main_board->get_you()->get_head())['y']) {
        return "down";
    }

    //safe return if everything above fails
    return "right";

}

void AlphaBeta::print_nodes_evaluated() {
    for (uint16_t i = 0; i < this->nodes_evaluated.size(); ++i) {
        std::cout << i << ":" << this->nodes_evaluated[i] << ' ';
    }
    std::cout << "\n";
}