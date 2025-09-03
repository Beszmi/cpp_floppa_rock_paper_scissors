#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include "game.hpp"
#include <ctime>

int main(int argc, char *argv[]) {
	const int fps_max = 120;
	const double target_dt = 1.0 / fps_max;
	srand(std::time(nullptr));

	Game game1;
	game1.init("EPIC FLOPPA ROCK PAPER SCISSORS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 3840, 2160, false);
	Uint64 now = SDL_GetPerformanceCounter();
	Uint64 last = now;
	const double freq = static_cast<double>(SDL_GetPerformanceFrequency());
	const double max_dt = 0.25;
	SDL_Cursor* default_cursor = SDL_GetDefaultCursor();
	SDL_Cursor* pointer_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
	game1.set_cursors(default_cursor, pointer_cursor);

	while (game1.running()) {
		now = SDL_GetPerformanceCounter();
		double dt = static_cast<double>(now - last) / freq;
		last = now;

		if (dt > max_dt) dt = max_dt;

		game1.handleEvents();
		game1.update(dt);
		game1.render();

		double frame_time = static_cast<double>(SDL_GetPerformanceCounter() - now) / freq;
		if (frame_time < target_dt) {
			Uint32 ms = static_cast<Uint32>((target_dt - frame_time) * 1000.0);
			if (ms > 0) SDL_Delay(ms);
		}
	}
	SDL_DestroyCursor(pointer_cursor);

	return 0;
}