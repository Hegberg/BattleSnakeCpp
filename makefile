CC = g++

CFLAGS = -pthread -std=c++17

REQUIRED = src/board.cpp src/snake.cpp src/alpha_beta.cpp src/board_evaluation.cpp src/flood_fill.cpp

all: test_main server_main

test_main:
	$(CC) $(CFLAGS) -DDEBUG -o test_app $(REQUIRED) test/test_main.cpp

server_main:
	$(CC) $(CFLAGS) -o main $(REQUIRED) main.cpp

clean:
	$(RM) test_app main
