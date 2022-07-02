#include "../inc/board.h"

Board::Board(nlohmann::json* board_data, nlohmann::json* you_data, int game_turn, std::string mode, std::string map) {
    this->width = board_data->at("width");
    this->height = board_data->at("height");
    this->turn = game_turn;
    this->game_mode = mode;
    this->game_map = map;

    this->you = new Snake(you_data);

    this->other_snakes = new std::vector<Snake*>;
    for (uint16_t i = 0; i < board_data->at("snakes").size(); ++i) {
        //skip self so don't have overlapping snakes between you and other_snakes
        if (board_data->at("snakes")[i]["id"] == you->get_id()) {
            continue;
        }
        Snake* snake = new Snake(&(board_data->at("snakes")[i]));
        this->other_snakes->push_back(snake);
    }

    this->dead_snakes = new std::vector<Snake*>;

    this->food = new std::map<std::string, std::map<char, int>* >;
    for (uint16_t i = 0; i < board_data->at("food").size(); ++i) {
        std::string co = std::to_string((int)board_data->at("food")[i]["x"]);
        co.append("_");
        co.append(std::to_string((int)board_data->at("food")[i]["y"]));
        //if food already not created
        if (this->food->find(co) == this->food->end()) {
            std::map<char, int>* coord = new std::map<char, int>;
            (*coord)['x'] = board_data->at("food")[i]["x"];
            (*coord)['y'] = board_data->at("food")[i]["y"];
            (*this->food)[co] = coord;
        }
    }

    this->depth = 0;
    this->alpha = -1.0;
    this->beta = 1.0;

    this->hazards = new std::map<std::string, std::map<char, int>* >;
    for (uint16_t i = 0; i < board_data->at("hazards").size(); ++i) {
        std::string co = std::to_string((int)board_data->at("hazards")[i]["x"]);
        co.append("_");
        co.append(std::to_string((int)board_data->at("hazards")[i]["y"]));
        //if didn't already create hazard for space
        if (this->hazards->find(co) == this->hazards->end()) {
            std::map<char, int>* coord = new std::map<char, int>;
            (*coord)['x'] = board_data->at("hazards")[i]["x"];
            (*coord)['y'] = board_data->at("hazards")[i]["y"];
            (*this->hazards)[co] = coord;
        }
    }

    this->dead_next_round_snakes = new std::vector<std::string>;

    this->children = new std::vector<Board*>;
    this->occupied_tiles = new std::map<std::string, bool>;
    create_occupied_tiles();
}

Board::Board(Board* old_board) {
    this->width = old_board->get_width();
    this->height = old_board->get_height();
    this->turn = old_board->get_turn();
    this->game_mode = old_board->get_game_mode();
    this->game_map = old_board->get_game_map();
    
    this->you = new Snake(old_board->get_you());

    //dead snakes from previous snake can be ignored, since they can just be removed for child
    this->dead_snakes = new std::vector<Snake*>;

    this->other_snakes = new std::vector<Snake*>;
    for (uint16_t i = 0; i < old_board->get_other_snakes()->size(); ++i) {
        //if snake is dead from previous turn eval, don't add to snakes, but add to dead snakes
        bool added_to_dead = false;
        for (uint16_t j = 0; j < old_board->get_dead_next_round_snakes()->size(); ++j) {
            if ((*old_board->get_other_snakes())[i]->get_id() == (*old_board->get_dead_next_round_snakes())[j]) {
                //don't add to dead snakes, since died from col if on dead next round snakes, so body gone this round
                //this->dead_snakes->push_back(new Snake(old_board->get_other_snakes()->at(i)));
                added_to_dead = true;
                break;
            }
        }
        if (added_to_dead) {continue;}
        this->other_snakes->push_back(new Snake(old_board->get_other_snakes()->at(i)));
    }

    this->food = new std::map<std::string, std::map<char, int>* >;
    std::map<std::string, std::map<char, int>*>::iterator it = old_board->get_food()->begin();
    while (it != old_board->get_food()->end()) {
        std::map<char, int>* coord = new std::map<char, int>;
        (*coord)['x'] = (*it->second)['x'];
        (*coord)['y'] = (*it->second)['y'];
        (*this->food)[it->first] = coord;
        ++it;
    }

    this->hazards = new std::map<std::string, std::map<char, int>* >;
    std::map<std::string, std::map<char, int>*>::iterator ith = old_board->get_hazards()->begin();
    while (ith != old_board->get_hazards()->end()) {
        std::map<char, int>* coord = new std::map<char, int>;
        (*coord)['x'] = (*ith->second)['x'];
        (*coord)['y'] = (*ith->second)['y'];
        (*this->hazards)[ith->first] = coord;
        ++ith;
    }

    this->occupied_tiles = new std::map<std::string, bool>;
    create_occupied_tiles();
    this->children = new std::vector<Board*>;

    this->dead_next_round_snakes = new std::vector<std::string>;

    this->depth = old_board->get_depth();
    this->alpha = -1.0;
    this->beta = 1.0;

    this->hazard_damage = old_board->hazard_damage;
}

Board::~Board() {
    for (uint16_t i = 0; i < other_snakes->size(); ++i) {
        delete (*other_snakes)[i];
    }
    delete other_snakes;
    for (uint16_t i = 0; i < dead_snakes->size(); ++i) {
        delete (*dead_snakes)[i];
    }
    delete dead_snakes;
    delete dead_next_round_snakes;
    delete you;
    delete occupied_tiles;
    for (uint16_t i = 0; i < children->size(); ++i) {
        delete (*this->children)[i];
    }
    delete children;
    std::map<std::string, std::map<char, int>*>::iterator it = this->food->begin();
    while (it != this->food->end()) {
        delete it->second;
        ++it;
    }
    delete this->food;
    std::map<std::string, std::map<char, int>*>::iterator ith = this->hazards->begin();
    while (ith != this->hazards->end()) {
        delete ith->second;
        ++ith;
    }
    delete this->hazards;
}

std::string Board::get_move_from_new_board(Board* new_board) {
    std::string next_move = "None";

    //compare self of new board to self of current board to get idea of where to go between 2 boards  
    if (new_board->you->get_head()['x'] > you->get_head()['x']) {
        next_move = "right";
        return next_move;
    } else if (new_board->you->get_head()['x'] < you->get_head()['x']) {
        next_move = "left";
        return next_move;
    } else if (new_board->you->get_head()['y'] > you->get_head()['y']) {
        next_move = "up";
        return next_move;
    } else if (new_board->you->get_head()['y'] < you->get_head()['y']) {
        next_move = "down";
        return next_move;
    }
    return next_move;
}

void Board::create_child_board_states() {

    children->clear();

    //check to make sure still alive
    if (this->get_you() == 0) {
        return;
    }

    Board* new_board = new Board(this);
    new_board->set_depth(this->depth + 1);
    children->push_back(new_board);

    std::vector<int> dead_snakes_index;
    std::vector<std::tuple<Snake*, int, int> > closest_snakes;

    uint16_t added_moves = 1;
    uint16_t allowed_child_board_limit = 27;
    //max boards 27, 3x3x3x1

    if (turn == 0) {
        //my snake, and closest snake, all possible moves (so 2 player mode don't lose out on starting duel moves)
        allowed_child_board_limit = 16;
    }

    uint8_t dead_snakes_skipped = 0;
    for (uint16_t i = 0; i < other_snakes->size(); ++i) {
        //check to see if snake is dead by eval in parent board, so don't include in live snakes for child
        bool skip = false;
        for (uint16_t j =0; j < dead_next_round_snakes->size(); ++j) {
            if ((*other_snakes)[i]->get_id() == (*dead_next_round_snakes)[j]) {
                dead_snakes_skipped++;
                skip = true;
                break;
            }
        }
        if (skip) {continue;}
        
        int distance = std::abs((*(*other_snakes)[i]->get_head())['x'] - (*you->get_head())['x'])
                     + std::abs((*(*other_snakes)[i]->get_head())['y'] - (*you->get_head())['y']);
        //std::pair<Snake*, int> snake_and_dist = {other_snakes->at(i), distance};
        closest_snakes.push_back({(*other_snakes)[i], distance, i - dead_snakes_skipped});
    }
    std::sort(closest_snakes.begin(), closest_snakes.end(), sort_by_sec);

    //add myself onto snake list first so I get evaluated before other snakes
    //closest_snakes.insert(closest_snakes.begin(), {this->you, 0});
    std::vector<std::pair<Snake*, int> > snake_list;
    snake_list.push_back({this->you, -1});
    for (uint16_t i = 0; i < closest_snakes.size(); ++i) {
        snake_list.push_back({std::get<0>(closest_snakes[i]), std::get<2>(closest_snakes[i])});
    }

    //for each snake, make a copy of existing boards, and for each copy modify the snake with possible moves
    for (uint16_t i = 0; i < snake_list.size(); ++i) {
        std::vector<std::pair<int,int> > pos_moves = get_possible_moves(new_board, snake_list[i].first);

        if (pos_moves.size() == 0) {
            dead_snakes_index.push_back(snake_list[i].second);
            continue;
        }
        
        //for moves that go over allowed limit, remove possible board children
        while (added_moves * pos_moves.size() > allowed_child_board_limit) {
            pos_moves.pop_back();
        }

        //update amount of added moves
        added_moves = added_moves * pos_moves.size();

        int snake_index = snake_list[i].second;
        std::map<char, int> new_head;
        //at least one move, so add to existing boards
        for (uint16_t j = 0; j < children->size(); ++j) {
            //snake is me
            if (snake_index == -1) {
                (*children)[j]->get_you()->pop_body_back();
                new_head['x'] = pos_moves[0].first;
                new_head['y'] = pos_moves[0].second;
                (*children)[j]->get_you()->add_new_head(new_head);
            } else {
                (*(*children)[j]->get_other_snakes())[snake_index]->pop_body_back();
                new_head['x'] = pos_moves[0].first;
                new_head['y'] = pos_moves[0].second;
                (*(*children)[j]->get_other_snakes())[snake_index]->add_new_head(new_head);
            }
        }

        //if more than 1 move, make copy of board states, and edit new board to contian new snake move
        std::vector<Board*> additional_children;
        for (uint16_t j = 1; j < pos_moves.size(); ++j) {
            for (uint16_t k = 0; k < children->size(); ++k) {
                Board* additional_board = new Board((*children)[k]);

                additional_board->set_depth(this->depth + 1);
                if (snake_index == -1) {
                    additional_board->get_you()->modify_head(pos_moves[j].first, pos_moves[j].second);
                } else {
                    (*additional_board->get_other_snakes())[snake_index]->modify_head(pos_moves[j].first, pos_moves[j].second);
                }
                additional_children.push_back(additional_board);
            }
        }

        for (uint16_t j = 0; j < additional_children.size(); ++j) {
            children->push_back(additional_children[j]);
        }
    }

    //remove dead snakes from board
    std::sort(dead_snakes_index.begin(), dead_snakes_index.end());
    for (uint16_t i = dead_snakes_index.size(); i > 0; --i) {
        for (uint16_t j = 0; j < children->size(); ++j) {
            (*children)[j]->remove_snake_at_index(dead_snakes_index[i-1]);
        }
    }

    //remove food from board that was eaten
    //update snakes eating the food
    for (uint16_t i = 0; i < children->size(); ++i) {
        std::vector<std::map<char,int> > food_to_remove;
        std::vector<int> food_dead_snake_index;

        //if still exist
        if ((*children)[i]->get_you() != 0) {
            (*children)[i]->find_food_being_eaten(food_to_remove, (*children)[i]->get_you());
            //check if ran out of health
            if ((*children)[i]->get_you()->get_health() <= 0) {
                (*children)[i]->remove_snake_at_index(-1);
            }
        }
        for (uint16_t j = 0; j < (*children)[i]->get_other_snakes()->size(); ++j) {
            (*children)[i]->find_food_being_eaten(food_to_remove, (*(*children)[i]->get_other_snakes())[j]);
            if ((*(*children)[i]->get_other_snakes())[j]->get_health() <= 0) {
                //insert at 0, so highest indexes get put first for proper snake removal
                food_dead_snake_index.insert(food_dead_snake_index.begin(), j);
            }
        }

        //remove snakes that have died from lack of food
        for (uint16_t j = 0; j < food_dead_snake_index.size(); ++j) {
            (*children)[i]->remove_snake_at_index(food_dead_snake_index[j]);
        }
    }

}

//make sure snake has more than 0 health before add to list of moves, to reduce moves added that are useless
std::vector<std::pair<int,int> > get_possible_moves(Board* board, Snake* snake) {
    std::vector<std::pair<int,int> > possible_moves;

    std::string coord = std::to_string((board->get_game_mode() == "wrapped" && (*snake->get_head())['x'] + 1 == board->get_width()) ? 0 : (*snake->get_head())['x'] + 1) + "_" + std::to_string((*snake->get_head())['y']);

    if (((*snake->get_head())['x'] + 1 < board->get_width() || board->get_game_mode() == "wrapped") && (board->get_occupied_tiles()->find(coord) == board->get_occupied_tiles()->end())) {
        
        std::pair<int, int> move = {(board->get_game_mode() == "wrapped" && (*snake->get_head())['x'] + 1 == board->get_width()) ? 0 : (*snake->get_head())['x'] + 1, (*snake->get_head())['y']};
        if (check_health(board, snake, move.first, move.second)) {
            possible_moves.push_back(move);
        }
    }

    coord = std::to_string((board->get_game_mode() == "wrapped" && (*snake->get_head())['x'] - 1 < 0) ? board->get_width() - 1 : (*snake->get_head())['x'] - 1) + "_" + std::to_string((*snake->get_head())['y']);

    if (((*snake->get_head())['x'] - 1 >= 0 || board->get_game_mode() == "wrapped") && (board->get_occupied_tiles()->find(coord) == board->get_occupied_tiles()->end())) {
        
        std::pair<int, int> move = {(board->get_game_mode() == "wrapped" && (*snake->get_head())['x'] - 1 < 0) ? board->get_width() - 1 : (*snake->get_head())['x'] - 1, (*snake->get_head())['y']};
        if (check_health(board, snake, move.first, move.second)) {
            possible_moves.push_back(move);
        }
    }

    coord = std::to_string((*snake->get_head())['x']) + "_" + std::to_string((board->get_game_mode() == "wrapped" && (*snake->get_head())['y'] + 1 == board->get_height()) ? 0 : (*snake->get_head())['y'] + 1);

    if (((*snake->get_head())['y'] + 1 < board->get_height() || board->get_game_mode() == "wrapped") && (board->get_occupied_tiles()->find(coord) == board->get_occupied_tiles()->end())) {
        
        std::pair<int, int> move = {(*snake->get_head())['x'], (board->get_game_mode() == "wrapped" && (*snake->get_head())['y'] + 1 == board->get_height()) ? 0 : (*snake->get_head())['y'] + 1};
        if (check_health(board, snake, move.first, move.second)) {
            possible_moves.push_back(move);
        }
    }

    coord = std::to_string((*snake->get_head())['x']) + "_" + std::to_string((board->get_game_mode() == "wrapped" && (*snake->get_head())['y'] - 1 < 0) ? board->get_height() - 1 : (*snake->get_head())['y'] - 1);

    if (((*snake->get_head())['y'] - 1 >= 0 || board->get_game_mode() == "wrapped") && (board->get_occupied_tiles()->find(coord) == board->get_occupied_tiles()->end())) {
        
        std::pair<int, int> move = {(*snake->get_head())['x'], (board->get_game_mode() == "wrapped" && (*snake->get_head())['y'] - 1 < 0) ? board->get_height() - 1 : (*snake->get_head())['y'] - 1};
        if (check_health(board, snake, move.first, move.second)) {
            possible_moves.push_back(move);
        }
    }

    return possible_moves;
}

//health rules
//1. every turn lose 1
//2. if hazard, reduce health by hazard damage
//3. if eat, health gets reset to 100 after standard decay
bool check_health(Board* board, Snake* snake, int x, int y) {
    int health = snake->get_health();
    health = health - 1;
    std::string location = std::to_string(x);
    location.append("_");
    location.append(std::to_string(y));
    if (board->is_hazard_at_location(location)) {
        health = health - board->get_hazard_damage();
    }
    if (board->is_food_at_location(location)) {
        health = 100;
    }
    return (health > 0);
}

void Board::create_occupied_tiles() {
    this->occupied_tiles->clear();
    //create_occupied_tiles, so every snake part not including tail (so -1 to skip tail)
    for (uint16_t i = 0; i < this->other_snakes->size(); ++i) {
        for (uint16_t j = 0; j < this->other_snakes->at(i)->get_body()->size() - 1; ++j) {
            std::string coord = std::to_string((*(*(*this->other_snakes)[i]->get_body())[j])['x']) + "_" + std::to_string((*(*(*this->other_snakes)[i]->get_body())[j])['y']);
            (*this->occupied_tiles)[coord] = true;
        }
    }
    //create_occupied_tiles, so every dead_snake part not including tail (so -1 to skip tail)
    for (uint16_t i = 0; i < this->dead_snakes->size(); ++i) {
        for (uint16_t j = 0; j < this->dead_snakes->at(i)->get_body()->size() - 1; ++j) {
            std::string coord = std::to_string((*(*(*this->dead_snakes)[i]->get_body())[j])['x']) + "_" + std::to_string((*(*(*this->dead_snakes)[i]->get_body())[j])['y']);
            (*this->occupied_tiles)[coord] = true;
        }
    }
    //create occupied tiles for myself not including tail (so -1 to skip tail)
    for (uint16_t i = 0; i < you->get_body()->size() - 1; ++i) {
        std::string coord = std::to_string((*(*you->get_body())[i])['x']) + "_" + std::to_string((*(*you->get_body())[i])['y']);
        (*this->occupied_tiles)[coord] = true;
    }
}

void Board::remove_snake_at_index(int index) {
    //remove self
    if (index == -1) {
        this->dead_snakes->push_back(this->you);
        this->you = 0;
    } else {
        //remove other snake
        this->dead_snakes->push_back(other_snakes->at(index));
        this->other_snakes->erase(other_snakes->begin() + index);
    }
}

void Board::find_food_being_eaten(std::vector<std::map<char,int> >& food_consumed, Snake* snake) {
    //health rules
    //1. every turn lose 1
    //2. if hazard, reduce health by hazard_damage
    //3. if eat, health gets reset to 100

    //regular health decay
    snake->remove_health(1);

    bool skip_removed_food = false;
    bool skip_hazard_check = false;
    //check if food exists, and if so, remove it and update snake
    std::map<std::string, std::map<char, int>*>::iterator it = this->food->begin();
    while (it != this->food->end()) {
        if ((*snake->get_head())['x'] == (*it->second)['x'] &&
            (*snake->get_head())['y'] == (*it->second)['y']) {
            delete it->second;
            this->food->erase(it->first);
            //only one food can overlap a person at a time, so if find food, safe to return after resetting health and not look for more
            food_consumed.push_back({{'x', (*snake->get_head())['x']}, {'y', (*snake->get_head())['y']}});
            snake->reset_health();
            //update len of snake and add tail
            snake->update_length(1);
            std::map<char, int>* new_tail = new std::map<char, int> {{'x', (*snake->get_body()->back())['x']}, {'y', (*snake->get_body()->back())['y']}};
            snake->add_new_tail(new_tail);
            skip_removed_food = true;
            skip_hazard_check = true;
            break;
        }
        ++it;
    }
    //check if would consume food that has already been consumed, and update snake
    if (!skip_removed_food) {
        for (uint16_t i = 0; i < food_consumed.size(); ++i) {
            if ((*snake->get_head())['x'] == food_consumed[i]['x'] &&
                (*snake->get_head())['y'] == food_consumed[i]['y']) {
                //food already removed, so just update snake health/len
                snake->reset_health();
                //update len of snake and add tail
                snake->update_length(1);
                std::map<char, int>* new_tail = new std::map<char, int> {{'x', (*snake->get_body()->back())['x']}, {'y', (*snake->get_body()->back())['y']}};
                snake->add_new_tail(new_tail);
                skip_hazard_check = true;
                break;
            }
        }
    }

    //if not eating, check if losing health to hazard
    if (!skip_hazard_check) {
        //check if new snake head in a hazard
        std::map<std::string, std::map<char, int>*>::iterator ith = this->hazards->begin();
        while (ith != this->hazards->end()) {
            if ((*snake->get_head())['x'] == (*ith->second)['x'] &&
                (*snake->get_head())['y'] == (*ith->second)['y']) {
                //if head in hazards and hasn't eaten, remove appropiate health
                snake->remove_health(this->hazard_damage);
                break;
            }
            ++ith;
        }
    }
}

void Board::set_depth(uint8_t new_depth) {
    this->depth = new_depth;
}

void Board::set_alpha(float new_alpha) {
    this->alpha = new_alpha;
}

void Board::set_beta(float new_beta) {
    this->beta = new_beta;
}

std::string Board::board_to_string() {
    std::string board_str;

    //if not dead
    if (this->you != 0) {
        board_str.append("you:");
        board_str.append(this->you->body_to_string());
        board_str.append(";");
    }

    for (uint16_t i = 0; i < this->other_snakes->size(); ++i) {
        board_str.append((*this->other_snakes)[i]->get_name());
        board_str.append(":");
        board_str.append((*this->other_snakes)[i]->body_to_string());
        board_str.append(";");
    }

    for (uint16_t i = 0; i < this->dead_snakes->size(); ++i) {
        board_str.append((*this->dead_snakes)[i]->get_name());
        board_str.append(":");
        board_str.append((*this->dead_snakes)[i]->body_to_string());
        board_str.append(";");
    }

    board_str.append("food:");
    std::map<std::string, std::map<char, int>*>::iterator it = this->food->begin();
    while (it != this->food->end()) {
        board_str.append(std::to_string((*it->second)['x']));
        board_str.append("_");
        board_str.append(std::to_string((*it->second)['y']));
        board_str.append(",");
        ++it;
    }
    board_str.append(";");

    return board_str;
}

bool Board::is_food_at_location(std::string loc) {
    std::map<std::string, std::map<char, int>*>::iterator it;
    it = this->food->find(loc);
    if (it != this->food->end()) {
        return true;
    }
    return false;
}

bool Board::is_hazard_at_location(std::string loc) {
    std::map<std::string, std::map<char, int>*>::iterator it;
    it = this->hazards->find(loc);
    if (it != this->hazards->end()) {
        return true;
    }
    return false;
}

void Board::print_board() {
    for (int i = this->get_height() - 1; i >= 0; --i) {
        for (int j = 0; j < this->get_width(); ++j) {
            std::map<char, int> coord = {{'x', j}, {'y', i}};
            bool found_snake = false;
            //other snakes
            for (uint16_t k = 0; k < this->get_other_snakes()->size(); ++k) {
                //head
                if (coord == *(this->get_other_snakes()->at(k)->get_head())) {
                    std::cout << (k+2) << '>';
                    found_snake = true;
                //tail
                } else if (coord == *(this->get_other_snakes()->at(k)->get_body()->at(this->get_other_snakes()->at(k)->get_body()->size() - 1))) {
                    std::cout << (k+2) << ')';
                    found_snake = true;
                //body
                } else {
                    for (uint16_t l = 0; l < this->get_other_snakes()->at(k)->get_body()->size(); ++l) {
                        if (coord == *(this->get_other_snakes()->at(k)->get_body()->at(l))) {
                            std::cout << (k+2) << ' ';
                            found_snake = true;
                            break;
                        }
                    }
                }

                if (found_snake) {
                    break;
                }
            }

            //you, and you exist
            if (!found_snake && this->get_you() != 0) {
                //head
                if (coord == *(this->get_you()->get_head())) {
                    std::cout << 0 << '>';
                    found_snake = true;
                //tail
                } else if (coord == *(this->get_you()->get_body()->at(this->get_you()->get_body()->size() - 1))) {
                    std::cout << 0 << ')';
                    found_snake = true;
                //body
                } else {
                    for (uint16_t l = 0; l < this->get_you()->get_body()->size(); ++l) {
                        if (coord == *(this->get_you()->get_body()->at(l))) {
                            std::cout << 0 << ' ';
                            found_snake = true;
                            break;
                        }
                    }
                }
            }

            if (found_snake) {
                continue;
            }

            bool found_food = false;
            std::string loc = std::to_string(coord.at('x'));
            loc.append("_");
            loc.append(std::to_string(coord.at('y')));

            if (this->is_food_at_location(loc)) {
                std::cout << "$ ";
                found_food = true;
            }

            if (found_food) {
                continue;
            }

            bool found_hazard = false;
            loc = std::to_string(coord.at('x'));
            loc.append("_");
            loc.append(std::to_string(coord.at('y')));

            if (this->is_hazard_at_location(loc)) {
                std::cout << "| ";
                found_hazard = true;
            }
            if (!found_hazard) {
                std::cout << "- ";
            }

        }
        std::cout << '\n';
    }
    std::cout << '\n';
}

bool sort_by_sec(const std::tuple<Snake*, int, int> &a, const std::tuple<Snake*, int, int> &b) {
    return (std::get<1>(a) < std::get<1>(b));
}