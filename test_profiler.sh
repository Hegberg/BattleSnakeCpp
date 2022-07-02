#!/bin/bash
rm test_app
echo This can take some time, there are over 30000 lines of code to compile in the libraries...
g++ -pthread -std=c++17 -pg -g -o test_app test/test_main.cpp src/board.cpp src/snake.cpp src/alpha_beta.cpp src/board_evaluation.cpp src/flood_fill.cpp src/common.cpp
./test_app
gprof ./test_app gmon.out > analysis.txt
