//HTTP credits: http://lcs.ios.ac.cn/~maxt/SPelton/reports/report-9aa0d3.html
//JSON credits: https://github.com/nlohmann/json
#include <thread>
#include <future>
#include <iostream>
#include <chrono>
#include <mutex>
#include <queue>
#include <map>
#include <string>
#include <utility>
#include <unistd.h>
#include <stdlib.h>
#include <fstream>
#include "./api/json.hpp"
#include "./api/http.h"
#include "inc/alpha_beta.h"
#include "inc/board.h"

//If want debug information, uncomment DEBUG define
//#define DEBUG

//If want games won/lost against snakes logged, uncomment LOG define
#define LOG

//If want multithreading enabled for server to handle multiple games at once, uncomment THREAD define
//#define THREAD

//For above defines, can edit Makefile to build with defines instead of commenting/uncommenting code above
//Test build automatically Defines DEBUG

using namespace std;
using namespace nlohmann;

uint16_t game_started = 0;
clock_t begin_time;
uint16_t job_counter = 0;
std::mutex thread_lock;
std::mutex response_lock;
std::queue<std::pair<uint16_t, nlohmann::json> > job_queue;
std::vector<std::pair<uint16_t, nlohmann::json> > response_queue;
std::map<std::string, std::pair<int, int> > snake_data;

string get_move(nlohmann::json data) {
  AlphaBeta alpha_beta = AlphaBeta(data);
  alpha_beta.breadth_first_search_loop();
  string move = alpha_beta.get_move_from_board();
  return move;
}

void infinite_loop_function(int thread_id) {
  std::pair<uint16_t, nlohmann::json> data (0,0);

  while (true) {
    
    if (job_queue.size() > 0) {
      thread_lock.lock();
      if (job_queue.size() > 0) {//if queue has not been modified since checked size and locked
        data = job_queue.front();
        job_queue.pop();
#ifdef DEBUG
        std::cout << "FUJ " << data.first << ' ' <<  float( clock () - begin_time ) /  CLOCKS_PER_SEC << '\n';
#endif
      }
      thread_lock.unlock();
    }

    if (data.second != 0) {
      string move = get_move(data.second);
      data.second = 0;
      response_lock.lock();
      std::pair<uint16_t, std::string> response (data.first, move);
      response_queue.push_back(response);
      response_lock.unlock();
#ifdef DEBUG
      std::cout << "FJ  " << data.first << ' ' <<  float( clock () - begin_time ) /  CLOCKS_PER_SEC << '\n';
#endif
    }
    
    if (game_started == 0) {
      std::chrono::microseconds dura(50);
      std::this_thread::sleep_for(dura);
    }
  }
}

int main(void) {
  //const clock_t begin_time = clock();
  httplib::Server svr;

#ifdef THREAD
  //float min_time_between_threads = 10.10f;
  //set up thread workers
  const int num_threads = std::thread::hardware_concurrency();
  std::vector<std::thread> threads;

  for (int i = 0; i < num_threads; i++)
  {
    threads.push_back(std::thread(infinite_loop_function, i));
  }
#endif

  //file format
  //snake_name
  //total_games
  //total_wins
  //snake_name 2
  //etc..

#ifdef LOG
  string line;
  string snake_name;
  uint16_t line_num = 0;
  fstream data_file;
  data_file.open ("data.txt");

  if (data_file.is_open()) {
    while (getline (data_file,line) ) {
      if (line_num == 0) {
        snake_name = line;
        snake_data[snake_name] = std::make_pair(1,0);
      } else if (line_num == 1) {
        snake_data[snake_name].first = std::stoi(line);
      } else if (line_num == 2) {
        snake_data[snake_name].second = std::stoi(line);
      }

      ++line_num;
      if (line_num > 2) {
        line_num = 0;
      }
    }
    data_file.close();
  }
#endif

  svr.Get("/", [](const auto &, auto &res) {
        std::cout << "GET\n";
        string head = "ski";
        string tail = "bolt";
        string author = "Whitishmeteor";
        string color = "#000000"; 
        res.set_content("{\"apiversion\":\"1\", \"head\":\"" + head + "\", \"tail\":\"" + tail + "\", \"color\":\"" + color + "\", " + "\"author\":\"" + author + "\"}", "text/json");
  });

  svr.Post("/end", [&](const auto &req, auto &res){
#ifdef LOG
    json data = json::parse(req.body);

    if (data["board"]["snakes"].size() > 0) {
      snake_data[data["board"]["snakes"][0]["name"]].second += 1;
    }

    std::remove("data.txt");
    ofstream data_file_write("data.txt");

    if (data_file_write.is_open()) {
      std::map<string, std::pair<int, int> >::iterator it = snake_data.begin();
      while (it != snake_data.end()) {
        data_file_write << it->first << '\n';
        data_file_write << it->second.first << '\n';
        data_file_write << it->second.second << '\n';
        ++it;
      }
      data_file_write.close();
    }
#endif
    game_started--;

    res.set_content("ok", "text/plain");
  });

  svr.Post("/start", [](const auto &req, auto &res){
#ifdef LOG
    json data = json::parse(req.body);
    for (uint16_t i = 0; i < data["board"]["snakes"].size(); ++i) {
      if (snake_data.find(data["board"]["snakes"][i]["name"]) != snake_data.end()) {
        snake_data[data["board"]["snakes"][i]["name"]].first += 1;
      } else {
        snake_data[data["board"]["snakes"][i]["name"]] = std::make_pair(1,0);
      }
    }
#endif
    game_started++;
    res.set_content("ok", "text/plain");
  });

  svr.Post("/move", [/*&turn_ended_time*/](auto &req, auto &res){
    json data = json::parse(req.body);
#ifdef DEBUG
    cout << data << "\n";
#endif

#ifndef THREAD

    //You can get the "you" property like this:
    //data["you"];
    //Almost alike python dictionary parsing, but with a semicolon at the end of each line.
    //You might need to make some structs to store some data in a variable
    //Example:
    //you_struct you = data["you"];

    AlphaBeta alpha_beta = AlphaBeta(data);
    alpha_beta.breadth_first_search_loop();
    string move = alpha_beta.get_move_from_board();
    res.set_content("{\"move\": \"" + move + "\"}", "text/plain");

#else

    thread_lock.lock();
    std::string move = "";
    uint16_t current_job = job_counter;
    std::pair<uint16_t, nlohmann::json> job (job_counter, data);
#ifdef DEBUG
    std::cout << "AJ  " << current_job << ' ' <<  float( clock () - begin_time ) /  CLOCKS_PER_SEC << '\n';
#endif
    job_counter++;
    job_queue.push(job);
    thread_lock.unlock();

    while (true) {
      if (response_queue.size() > 0) {
        std::pair<uint16_t, std::string> response;
        response_lock.lock();
        for (uint16_t i = 0; i < response_queue.size(); ++ i) {
          if (response_queue.at(i).first == current_job) {
            move = response_queue.at(i).second;
            res.set_content("{\"move\": \"" + move + "\"}", "text/plain");
            response_queue.erase(response_queue.begin() + i);
#ifdef DEBUG
            std::cout << "FCJ " << current_job << ' ' <<  float( clock () - begin_time ) /  CLOCKS_PER_SEC << ' ' << move << "\n\n";
#endif
            break;
          }
        }
        response_lock.unlock();
        if (move != "") {
          break;
        }
      }
      //sleep(1);
    }
#endif

  });

  //modify to wahtever locally hosted server information, or relevant web hosted information
  svr.listen("192.168.1.1", 8080);

  //return 1 so server.sh loops on program end (aka restart server on program crash)
  //current implementation doesn't crash and doesn't have any memory leaks, but good to have this up if trying out new untested code
  return 1;
}
