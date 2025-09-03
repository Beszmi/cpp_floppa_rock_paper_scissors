#pragma once
#ifndef gameplay_hpp
#define gameplay_hpp
#include <iostream>
#include <fstream>
#include <vector>
#include <string>


// 0 rock; 1 paper; 2 scissors
namespace rps {
	int play(int input);
}

struct player_stat {
	std::string name = "epic_gamer";
	size_t wins, losses, draws;
	player_stat(std::string name, int wins = 0, int draws = 0, int loses = 0);
	void add_stat(int input);
};

class player_container {
	std::vector<std::unique_ptr<player_stat>> players;
	size_t current_player;
public:
	player_container();

	std::unique_ptr<player_stat>& get_player(size_t idx) { return players.at(idx); }
	std::unique_ptr<player_stat>& operator[](size_t idx) { return players.at(idx); }
	size_t get_current_player_id() { return current_player; }
	void set_current_player_id(size_t idx) { current_player = idx; }
	size_t get_size() { return players.size(); }

	void add_new_player(player_stat new_player);

	~player_container() = default;
};

namespace file_managemenet {
	std::vector<std::string> split(const std::string& str, char delimiter);
	void read_data(player_container& players);
	void write_data(player_container& players);
}

#endif