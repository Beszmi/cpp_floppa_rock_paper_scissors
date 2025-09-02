#pragma once
#ifndef gameplay_hpp
#define gameplay_hpp
#include <iostream>


// 0 rock; 1 paper; 2 scissors
namespace rps {
	int play(int input);
}

class player_stats {
	std::string name = "epic_gamer";
	size_t wins, losses, draws;
public:
	void add_stat(int input);
};

#endif