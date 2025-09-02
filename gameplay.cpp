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

void player_stats::add_stat(int input) {
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