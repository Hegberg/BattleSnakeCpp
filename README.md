# A [Battlesnake](http://play.battlesnake.com) written in C++.

This is a basic implementation of the [Battlesnake API](https://docs.battlesnake.com/snake-api). It's a great starting point for anyone who has experimented with Battlesnake before and wants a solid base for a more advanced snake.

## Implementation

This snake has a C++ base for a Min/Max or Alpha/Beta based snake. It's algorithm for calculating the value of a given board state has been taken out to prevent clones of my snake appearing on the tournament ladders, and to encourage people to understand the code base, while allowing for creative approaches to a new board value calculator. The code properly recreates future board states following the battlesnake server rules, as well as collision, hazard and food handling.

The code includes multiple different files to assist in creating a battlesnake, including multiple types of makefiles, a test suite with 100+ test cases and even a code profiler to assist with finding areas where the code is bottlenecking and underperforming. The server itself also features a debug mode, log mode, and multi-threading mode, allowing for basic debugging, stat collection, and a simple multi-threading implementation to help any developers struggling with multi-threading their battlesnake server to allow for a battlesnake server to handle multiple games at the same time. 

The current implementation is memory safe. If you find yourself adding new code with more memory allocation, using GDB plus the test suite will easily check for any memory loss in new code.

## Results

Currently this Battlesnake runs in the 4 Seasonal Leagues year round which are divided by Elite/Platinum/Gold/Silver/Everyone Else. Elite is the top 8 or 16 snakes of 100+. My snake has finished every League in Elite. With varying [results](https://play.battlesnake.com/u/hegberg/#achievements) in the end of league tournaments, with a few tournament wins in the early leagues.

## Questions?

All documentation is available at [docs.battlesnake.com](https://docs.battlesnake.com), including detailed Guides, API References, and Tips.

You can also join the Battlesnake Developer Community on [Discord](https://play.battlesnake.com/discord). We have a growing community of Battlesnake developers of all skill levels wanting to help everyone succeed and have fun with Battlesnake. You can also contact me there with any direct questions.
