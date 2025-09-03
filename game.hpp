#pragma once

#ifndef game_h
#define game_h
#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "graphic_components/texture_manager.hpp"
#include "game_obj.hpp"
#include "graphic_components/camera.hpp"
#include "text.hpp"

class Game {
	bool run;
	//int cnt = 0;
	SDL_Window* window;
	SDL_Renderer* renderer;
	texture_manager tex_mgr;
	Game_obj_container obj_container;
	int screen_w, screen_h;
	Camera cam;
	float screen_scale_factor = 1.0f;

	GameObject* hovered = nullptr;
	//GameObject* held_obj = nullptr;
	//int held_button = 0;
	Uint64 hold_t0_ms = 0;
	bool holding = false;
	SDL_Cursor* default_cursor;
	SDL_Cursor* pointer_cursor;
	bool need_update = false;
	int result;
	player_container players;
	int current_scene = 0;
public:
	Game();
	~Game();

	void init(const char *title, int xpos, int ypos, int width, int height, bool fullscreen);
	void set_cursors(SDL_Cursor* default_cursor_in, SDL_Cursor* pointer_cursor_in);
	void handleEvents();
	void update(double dtSeconds);
	void render();
	void clean();
	SDL_Cursor* get_def_cursor() { return default_cursor; }
	SDL_Cursor* get_pointer_cursor() { return pointer_cursor; }

	bool running() const;
};


#endif // !game_h
