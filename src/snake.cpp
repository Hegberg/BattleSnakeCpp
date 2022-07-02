#include "../inc/snake.h"

Snake::Snake(nlohmann::json* data) {
    this->id = data->at("id");
    this->name = data->at("name");
    this->health = data->at("health");
    this->length = data->at("length");

    this->head = new std::map<char, int>;
    (*this->head)['x'] = data->at("head")["x"];
    (*this->head)['y'] = data->at("head")["y"];

    this->body = new std::vector<std::map<char, int>* >;
    for (uint16_t i = 0; i < data->at("body").size(); ++i) {
        std::map<char, int>* coord = new std::map<char, int>;
        (*coord)['x'] = data->at("body")[i]["x"];
        (*coord)['y'] = data->at("body")[i]["y"];
        this->body->push_back(coord);
    }
}

Snake::Snake(Snake* old_snake) {
    this->id = old_snake->get_id();
    this->name = old_snake->get_name();
    this->shout = old_snake->get_shout();
    this->health = old_snake->get_health();
    this->length = old_snake->get_length();

    this->head = new std::map<char, int>;
    (*this->head)['x'] = (*old_snake->get_head())['x'];
    (*this->head)['y'] = (*old_snake->get_head())['y'];

    this->body = new std::vector<std::map<char, int>* >;
    for (uint16_t i = 0; i < old_snake->get_body()->size(); ++i) {
        std::map<char, int>* coord = new std::map<char, int>;
        (*coord)['x'] = (*(*old_snake->get_body())[i])['x'];
        (*coord)['y'] = (*(*old_snake->get_body())[i])['y'];
        this->body->push_back(coord);
    }
}

Snake::~Snake() {
    delete this->head;
    for (uint16_t i = 0; i < body->size(); ++i) {
        delete this->body->at(i);
    }
    delete this->body;
}

void Snake::pop_body_back() {
    std::map<char, int>* back_pointer = this->body->at(body->size() - 1);
    body->pop_back();
    delete back_pointer;
}

void Snake::add_new_head(std::map<char, int> new_head) {
    (*this->head)['x'] = new_head['x'];
    (*this->head)['y'] = new_head['y'];
    std::map<char, int>* coord = new std::map<char, int>;
    (*coord)['x'] = new_head['x'];
    (*coord)['y'] = new_head['y'];
    this->body->insert(this->body->begin(), coord);
}

void Snake::modify_head(int x, int y) {
    (*this->head)['x'] = x;
    (*this->head)['y'] = y;
    (*(*this->body)[0])['x'] = x;
    (*(*this->body)[0])['y'] = y;
}

void Snake::remove_health(int removed_health) {
    this->health = this->health - removed_health;
}

void Snake::update_length(int additional_len) {
    this->length = this->length + additional_len;
}

void Snake::add_new_tail(std::map<char, int>* new_tail) {
    this->body->push_back(new_tail);
}

bool Snake::find_body(int x, int y) {
    for (uint16_t i = 0; i < this->body->size(); ++i) {
        if ((*(*this->body)[i])['x'] == x && (*(*this->body)[i])['y'] == y) {
            return true;
        }
    }
    return false;

}

std::string Snake::body_to_string() {
    std::string body_str;
    for (uint16_t i = 0; i < this->body->size(); ++i) {
        body_str.append(std::to_string((*(*this->body)[i])['x']));
        body_str.append("_");
        body_str.append(std::to_string((*(*this->body)[i])['y']));
        body_str.append(",");
    }
    return body_str;
}