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

class Snake {
    private:
        std::string id;
        std::string name;
        std::string shout;
        int length;
        int health;
        std::vector<std::map<char, int>* >* body;
        std::map<char, int>* head;
    
    public:
        Snake(nlohmann::json*);
        Snake(Snake*);
        ~Snake();
        
        std::string get_id() {return id;}
        std::string get_name() {return name;}
        std::string get_shout() {return shout;}
        int get_length() {return length;}
        int get_health() {return health;}
        std::vector<std::map<char, int>* >* get_body() {return body;}
        std::map<char, int>* get_head() {return head;}
        std::map<char, int>* get_tail() {return body->at(body->size() - 1);}
        
        void pop_body_back();
        void add_new_head(std::map<char, int>);
        void add_new_tail(std::map<char, int>*);
        void modify_head(int, int);
        void reset_health() {this->health = 100;}
        void remove_health(int);
        void update_length(int);
        bool find_body(int, int);
        std::string body_to_string();
};