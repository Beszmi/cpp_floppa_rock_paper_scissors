#include "gameplay.hpp"

int rps::play(int input) {
	int cpu_item = rand() % 3;
	if ((input == 0 && cpu_item == 2) || (input == 1 && cpu_item == 0) || (input == 2 && cpu_item == 1)) {
		return 1; //WIN
	} else if (input == cpu_item) {
		return 0; //DRAW
	} else if ((input == 2 && cpu_item == 0) || (input == 0 && cpu_item == 1) || (input == 1 && cpu_item == 2)) {
		return -1; //LOSE
	}
	else {
		std::cout << "error in play()" << std::endl;
		return -2;
	}
}

player_stat::player_stat(std::string name, int wins, int draws, int loses): name(name), wins(wins), draws(draws), losses(loses) {}

void player_stat::add_stat(int input) {
	switch (input)
	{
	case 1:
		wins++;
		break;
	case 0:
		draws++;
		break;
	case -1:
		losses++;
		break;
	default:
		std::cout << "add_stat error" << std::endl;
		break;
	}
}

player_container::player_container(): current_player(0) {}

void player_container::add_new_player(player_stat new_player) {
	auto p = std::make_unique<player_stat>(new_player);
	players.push_back(std::move(p));
}

bool file_managemenet::read_data(player_container& players) {
	using namespace std;
	ifstream file("data/player_data.csv");
}

bool file_managemenet::write_data(player_container& players) {

}
