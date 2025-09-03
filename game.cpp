#include "game.hpp"

Game::Game() : tex_mgr(nullptr), screen_w(0), screen_h(0), renderer(nullptr), window(nullptr), run(false), default_cursor(nullptr), pointer_cursor(nullptr) {}

Game::~Game() { clean(); }

namespace {
	bool GetWindowBorders(int& top, int& bottom) {
		top = bottom = 0;

		SDL_Window* test = SDL_CreateWindow("test", 64, 64, 0);
		if (!test) return false;

		SDL_ShowWindow(test);

		const Uint64 start = SDL_GetTicksNS();
		bool status = false;
		while ((SDL_GetTicksNS() - start) < 50'000'000ULL) {
			SDL_PumpEvents();
			status = SDL_GetWindowBordersSize(test, &top, nullptr, &bottom, nullptr);
			if (status) break;
			SDL_Delay(1);
		}

		SDL_DestroyWindow(test);
		return status;
	}
}

void Game::init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen) {
	int flags = fullscreen ? SDL_WINDOW_FULLSCREEN : 0;

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
		run = false;
		return;
	}

	int target_w = width;
	int target_h = height;
	int target_x = -1, target_y = -1;
	SDL_WindowFlags wflags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
	if (fullscreen) {
		wflags |= SDL_WINDOW_FULLSCREEN;
	}
	else {
		wflags |= SDL_WINDOW_RESIZABLE;
		int topBorder = 0, bottomBorder = 0;
		GetWindowBorders(topBorder, bottomBorder);

		if (SDL_DisplayID did = SDL_GetPrimaryDisplay()) {
			SDL_Rect usable{};
			if (SDL_GetDisplayUsableBounds(did, &usable)) {
				target_w = usable.w;
				target_h = usable.h - topBorder;
				target_x = usable.x;
				target_y = usable.y + topBorder;
			}
			else {
				std::cerr << "SDL_GetDisplayUsableBounds failed: " << SDL_GetError() << "\n";
			}
		}
		else {
			std::cerr << "SDL_GetPrimaryDisplay failed: " << SDL_GetError() << "\n";
		}
	}
	//---------------------------------------------------
	//SDL_SetHint(SDL_HINT_RENDER_DRIVER, "vulkan");
	//---------------------------------------------------
	if (!SDL_CreateWindowAndRenderer(title, target_w, target_h, wflags, &window, &renderer)) {
		std::cerr << "SDL_CreateWindowAndRenderer failed: " << SDL_GetError() << "\n";
		run = false;
		return;
	}

	if (target_w <= 1280 && target_h <= 720) {
		screen_scale_factor = 0.75;
	}
	else if (target_w <= 1920 && target_h <= 1080) {
		screen_scale_factor = 1;
	}
	else if (target_w <= 2560 && target_h <= 1440) {
		screen_scale_factor = 1.5;
	}
	else if (target_w <= 3840 && target_h <= 2160) {
		screen_scale_factor = 2;
	}

	std::cout << "starting window size: width:" << target_w << " height: " << target_h << std::endl;
	std::cout << "screen scale factor: " << screen_scale_factor << std::endl;

	if (!fullscreen && target_x >= 0 && target_y >= 0) {
		SDL_SetWindowPosition(window, target_x, target_y);
		run = false;
	}

	if (!SDL_SetRenderVSync(renderer, SDL_RENDERER_VSYNC_ADAPTIVE)) {
		//if adaptive not available unlimited fps
		SDL_SetRenderVSync(renderer, 0);
	}
	SDL_SetWindowMinimumSize(window, 640, 360);
	SDL_SetWindowMaximumSize(window, 3840, 2160);
	std::cout << "SDL init + window/renderer created!\n";
	SDL_GetWindowSizeInPixels(window, &screen_w, &screen_h);

	if (!TTF_Init()) {
		std::cerr << "TTF_Init failed: " << SDL_GetError() << "\n";
		run = false;
		return;
	}

	tex_mgr.set_renderer(renderer);
	tex_mgr.load_textures_from_folder("assets");
	tex_mgr.load_textures_from_folder("assets/sprites");

	file_managemenet::read_data(players);
	players.set_current_player_id(1);

	//Screen space sizes
	SDL_FPoint middle = ScreenPercentToWindow(renderer, 0.5f, 0.5f);
	SDL_FPoint fifth = ScreenPercentToWindow(renderer, 0.2f, 0.2f);
	SDL_FPoint tenth = ScreenPercentToWindow(renderer, 0.1f, 0.1f);
	SDL_FPoint percent = ScreenPercentToWindow(renderer, 0.01f, 0.01f);

	//perma layer
	obj_container.spawn_as<streched_bg_obj>("-", "-", tex_mgr, 1.0f, true, -1);
	streched_bg_obj& floppa = *obj_container.get<streched_bg_obj>("-");
	floppa.init("floppa", tex_mgr, screen_w, screen_h);

	tex_mgr.create_text_texture("title", "fonts/ARIAL.TTF", 72, "FLOPPA ROCK PAPER SCISSORS", Colors::red);
	obj_container.spawn_as<Button>("title", "title", tex_mgr, middle.x - ((tex_mgr.get_texture("title")->w / 2) * screen_scale_factor), (middle.y - (tex_mgr.get_texture("title")->h / 2)) / 16, screen_scale_factor, true, 10);

	tex_mgr.create_text_texture("finish_text", "fonts/ARIAL.TTF", 48, "SAVE & QUIT", Colors::red);
	tex_mgr.set_text_background("finish_text", true, Colors::white, 4, 4);
	tex_mgr.set_text_border("finish_text", true, Colors::black, 2);
	obj_container.spawn_as<Text_Button>("finish_text", "finish_text", tex_mgr, percent.x, (screen_h - (2 * tex_mgr.get_texture("finish_text")->h) - percent.y), screen_scale_factor, true, 9, 4);

	tex_mgr.create_text_texture("player_name_text", "fonts/ARIAL.TTF", 64, "Logged in as: " + players.get_player(players.get_current_player_id())->name, Colors::black);
	tex_mgr.set_text_background("player_name_text", true, Colors::white_seethru, 4, 4);
	obj_container.spawn_as<Text_Button>("player_name_text", "player_name_text", tex_mgr, percent.y, screen_scale_factor, true, 3, 0);
	//------------------------------------------------------

	//main menu
	obj_container.spawn_as<streched_bg_obj>("kadfloppa", "-", tex_mgr, 1.0f, true, 4);
	streched_bg_obj& kadfloppa = *obj_container.get<streched_bg_obj>("kadfloppa");
	kadfloppa.init("kadfloppa", tex_mgr, screen_w, screen_h);

	obj_container.spawn_as<GameObject>("menu", "menu", tex_mgr, middle.x - ((tex_mgr.get_texture("menu")->w / 2) * screen_scale_factor), middle.y / 2, screen_scale_factor,true, 5);

	tex_mgr.create_text_texture("start_text", "fonts/ARIAL.TTF", 48, "PLAY", Colors::white);
	tex_mgr.set_text_background("start_text", true, Colors::light_grey, 4, 4);
	tex_mgr.set_text_border("start_text", true, Colors::black, 2);
	obj_container.spawn_as<Text_Button>("start_text", "start_text", tex_mgr, middle.x + (middle.x / 2), middle.y + (middle.y / 3), screen_scale_factor, true, 6, 5);

	size_t index = 0;
	for (index = 0; index < players.get_vector().size(); index++) {
		tex_mgr.create_text_texture("play_text" + index, "fonts/ARIAL.TTF", 48, players[index]->name, Colors::white);
		tex_mgr.set_text_background("play_text" + index, true, Colors::light_grey, 4, 4);
		tex_mgr.set_text_border("play_text" + index, true, Colors::black, 2);
		obj_container.spawn_as<Text_Button>("play_text" + index, "play_text" + index, tex_mgr, (middle.x - 1.6 * fifth.x), ((middle.y - 1.2 * fifth.y) + (tex_mgr.get_texture("play_text" + index)->h) * (2.5*index)), screen_scale_factor, true, 6, 100+index);
		cout << players[index]->name << "\n";
	}
	//------------------------------------------------------

	//play layer
	tex_mgr.create_text_texture("rock_text", "fonts/ARIAL.TTF", 48, "ROCK", Colors::white);
	tex_mgr.set_text_background("rock_text", true, Colors::light_grey, 4, 4);
	tex_mgr.set_text_border("rock_text", true, Colors::black, 2);
	obj_container.spawn_as<Text_Button>("rock_text", "rock_text", tex_mgr, middle.x - (middle.x / 2) - ((tex_mgr.get_texture("rock_text")->w / 2) * screen_scale_factor), (middle.y - (tex_mgr.get_texture("rock_text")->h / 2)) * 1.5, screen_scale_factor, true, 0, 0);

	tex_mgr.create_text_texture("paper_text", "fonts/ARIAL.TTF", 48, "PAPER", Colors::white);
	tex_mgr.set_text_background("paper_text", true, Colors::light_grey, 4, 4);
	tex_mgr.set_text_border("paper_text", true, Colors::black, 2);
	obj_container.spawn_as<Text_Button>("paper_text", "paper_text", tex_mgr, middle.x - ((tex_mgr.get_texture("paper_text")->w / 2) * screen_scale_factor), (middle.y - (tex_mgr.get_texture("paper_text")->h / 2)) * 1.5, screen_scale_factor, true, 0, 1);

	tex_mgr.create_text_texture("scissors_text", "fonts/ARIAL.TTF", 48, "SCISSORS", Colors::white);
	tex_mgr.set_text_background("scissors_text", true, Colors::light_grey, 4, 4);
	tex_mgr.set_text_border("scissors_text", true, Colors::black, 2);
	obj_container.spawn_as<Text_Button>("scissors_text", "scissors_text", tex_mgr, middle.x + (middle.x / 2) - ((tex_mgr.get_texture("scissors_text")->w / 2) * screen_scale_factor), (middle.y - (tex_mgr.get_texture("scissors_text")->h / 2)) * 1.5, screen_scale_factor, true, 0, 2);
	//------------------------------------------------------

	//results layer
	tex_mgr.create_text_texture("extra_text", "fonts/ARIAL.TTF", 48, "PLAY AGAIN", Colors::white);
	tex_mgr.set_text_background("extra_text", true, Colors::light_grey, 4, 4);
	tex_mgr.set_text_border("extra_text", true, Colors::black, 2);
	obj_container.spawn_as<Text_Button>("extra_text", "extra_text", tex_mgr, middle.x + (middle.x / 4), middle.y + (middle.y / 12), screen_scale_factor, false, 2, 3);

	obj_container.spawn_as<sprite>("explosion", "-", tex_mgr, middle.x - (middle.x / 2), middle.y - (middle.y / 3 + 3*percent.y), screen_scale_factor * 0.2, false, 2);

	sprite& explosion = *obj_container.get<sprite>("explosion");

	for (int i = 1;; ++i) {
		std::string key = "sprites/s" + std::to_string(i);
		if (!tex_mgr.has(key)) break;
		explosion.add_element(key, tex_mgr);
	}

	obj_container.spawn_as<GameObject_cluster>("design", "design", tex_mgr, middle.x - ((tex_mgr.get_texture("title")->w / 2) * screen_scale_factor), (middle.y - (tex_mgr.get_texture("title")->h / 2)) / 3, screen_scale_factor, false, 1);

	tex_mgr.create_text_texture("result_text", "fonts/ARIAL.TTF", 48, "SCORE: NONE", Colors::black);
	obj_container.spawn_as<Text_Button>("result_text", "result_text", tex_mgr, 0, 0, screen_scale_factor, false, -1);

	tex_mgr.create_text_texture("win_counter", "fonts/ARIAL.TTF", 48, "0", Colors::black);
	obj_container.spawn_as<Text_Button>("win_counter", "win_counter", tex_mgr, 0, 0, screen_scale_factor, false, -1);

	tex_mgr.create_text_texture("tie_counter", "fonts/ARIAL.TTF", 48, "0", Colors::black);
	obj_container.spawn_as<Text_Button>("tie_counter", "tie_counter", tex_mgr, 0, 0, screen_scale_factor, false, -1);

	tex_mgr.create_text_texture("lose_counter", "fonts/ARIAL.TTF", 48, "0", Colors::black);
	obj_container.spawn_as<Text_Button>("lose_counter", "lose_counter", tex_mgr, 0, 0, screen_scale_factor, false, -1);

	GameObject_cluster& box = *obj_container.get<GameObject_cluster>("design");
	box.add_item_local(*obj_container.get("result_text"), 2* percent.x, 3* percent.y, true);
	box.add_item_local(*obj_container.get("win_counter"), (middle.x - 5*percent.x), 3 * percent.y, true);
	box.add_item_local(*obj_container.get("tie_counter"), (middle.x - 5 * percent.x), (tenth.y + 1.5*percent.y), true);
	box.add_item_local(*obj_container.get("lose_counter"), (middle.x - 5 * percent.x), (2*tenth.y - percent.y), true);
	//------------------------------------------------------

	run = true;
}

void Game::set_cursors(SDL_Cursor* default_cursor_in, SDL_Cursor* pointer_cursor_in) {
	default_cursor = default_cursor_in;
	pointer_cursor = pointer_cursor_in;
}

void Game::handleEvents() {
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_EVENT_QUIT:
			run = false;
			break;
		case SDL_EVENT_KEY_DOWN:
			if (!e.key.repeat && e.key.scancode == SDL_SCANCODE_ESCAPE) {
				run = false;
			}
			break;
		case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
			SDL_GetWindowSizeInPixels(window, &screen_w, &screen_h);
			break;
		case SDL_EVENT_WINDOW_RESIZED:
			SDL_GetWindowSizeInPixels(window, &screen_w, &screen_h);
			if (screen_w <= 1280 && screen_h <= 720) {
				screen_scale_factor = 0.75;
			}
			else if (screen_w <= 1920 && screen_h <= 1080) {
				screen_scale_factor = 1;
			}
			else if (screen_w <= 2560 && screen_h <= 1440) {
				screen_scale_factor = 1.5;
			}
			else if (screen_w <= 3840 && screen_h <= 2160) {
				screen_scale_factor = 2;
			}
			obj_container.set_scale_all(screen_scale_factor);
			break;
		case SDL_EVENT_MOUSE_MOTION: {
			SDL_FPoint W = WindowToWorld(renderer, e.motion.x, e.motion.y, cam);
			//mouse.dx = e.motion.xrel, mouse.dy = e.motion.yrel; //currently unused commented out for performance
			GameObject* hit = obj_container.pick_topmost(W.x, W.y);

			if (hit != hovered) {
				if (hovered) hovered->on_hover_exit(default_cursor);
				hovered = hit;
				if (hovered) hovered->on_hover_enter(pointer_cursor);
			}
			else if (hovered) {
				hovered->on_hover();
			}
		}	break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN: {}
			break;
		case SDL_EVENT_MOUSE_BUTTON_UP: {
			if (e.button.button == SDL_BUTTON_LEFT) {
				SDL_FPoint W = WindowToWorld(renderer, e.button.x, e.button.y, cam);
				if (auto* hit = obj_container.pick_topmost(W.x, W.y)) {
					result = hit->action();
					if (static_cast<Text_Button*>(hit)->is_enabled()) {
						if (result >= -1 && result < 2) {
							players.get_player(players.get_current_player_id())->add_stat(result);
							need_update = true;
							current_scene = 1;
						}
					}
					if (result == 3 || result == 5) { //to play scene
						need_update = true;
						current_scene = 0;
					}
					if (result == 4) {
						need_update = true;
						current_scene = -1;
					}
					if (result > 99) {
						need_update = true;
						current_scene = 2;
						if (result - 100 > players.get_size()) {
							std::cerr << "user range exceeded" << std::endl;
						}
						else {
							players.set_current_player_id(result - 100);
							obj_container.get<Text_Button>("play_text" + (result - 100))->set_background(true, Colors::dark_green);
							size_t index = 0;
							for (index = 0; index < players.get_vector().size(); index++) {
								if (index != result - 100) {
									tex_mgr.set_text_background("play_text" + index, true, Colors::light_grey, 4, 4);
								}
							}
						}
					}
				}
			}
			//mouse.clicks[b] = e.button.clicks;
		}	break;
		case SDL_EVENT_MOUSE_WHEEL: {
			const float step = 1.1f; // 1.05–1.2
			const float z0 = cam.zoom;
			const float z1 = std::clamp(z0 * std::pow(step, e.wheel.y), 0.1f, 5.0f);

			float mx_i, my_i; SDL_GetMouseState(&mx_i, &my_i);
			float rx, ry;
			WindowToRender(renderer, mx_i, my_i, rx, ry);

			// keep cursor anchored: ΔC = r * (1 - z0/z1), with r in render coords
			cam.x += rx * (1.0f - z0 / z1);
			cam.y += ry * (1.0f - z0 / z1);
			cam.zoom = z1;
		}	break;
		default:
			break;
		}
	}
}


void Game::update(double dtSeconds) {
	//cnt++;
	if (need_update) {
		player_stat& active_player = *players.get_player(players.get_current_player_id());
		Text_Button& score_text = *obj_container.get<Text_Button>("result_text");
		if (result == 1) {
			score_text.set_text("YOU WON!");
			obj_container.get<Text_Button>("win_counter")->set_text(std::to_string(active_player.wins));
		} else if (result == 0) {
			score_text.set_text("YOU TIE!");
			obj_container.get<Text_Button>("tie_counter")->set_text(std::to_string(active_player.draws));
		} else if (result == -1) {
			score_text.set_text("you lose :(");
			obj_container.get<Text_Button>("lose_counter")->set_text(std::to_string(active_player.losses));
		}

		if (current_scene == 1) { // results screen
			obj_container.layer_switch(4, false);
			obj_container.layer_switch(1, true);
			obj_container.layer_switch(2, true);
			obj_container.get<Text_Button>("rock_text")->switch_enable(false);
			obj_container.get<Text_Button>("paper_text")->switch_enable(false);
			obj_container.get<Text_Button>("scissors_text")->switch_enable(false);
		} 
		else if (current_scene == 0) { //play screen
			obj_container.layer_switch(0, true);
			obj_container.layer_switch(1, false);
			obj_container.layer_switch(2, false);
			obj_container.layer_switch(4, false);
			obj_container.layer_switch(5, false);
			obj_container.layer_switch(6, false);
			obj_container.get<Text_Button>("rock_text")->switch_enable(true);
			obj_container.get<Text_Button>("paper_text")->switch_enable(true);
			obj_container.get<Text_Button>("scissors_text")->switch_enable(true);
		} 
		else if (current_scene == -1) { //quit
			obj_container.layer_switch(4, false);
			obj_container.layer_switch(0, false);
			obj_container.layer_switch(1, false);
			obj_container.layer_switch(2, false);
			obj_container.layer_switch(9, false);
			file_managemenet::write_data(players);
			run = false;
		}
		else if (current_scene == 2) { //main menu
			obj_container.layer_switch(4, true);
			obj_container.layer_switch(1, false);
			obj_container.layer_switch(2, false);
			obj_container.layer_switch(9, true);
			obj_container.layer_switch(10, true); //title
			obj_container.get<Text_Button>("player_name_text")->set_text("Logged in as: " + players.get_player(players.get_current_player_id())->name);
		}
		obj_container.rebuild_order();
		need_update = false;
		std::cout << "current results for " << active_player.name << ": wins: " << active_player.wins << " draws: " << active_player.draws << " losses: " << active_player.losses << std::endl;
	}
	obj_container.update_all(dtSeconds);
}

void Game::render() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	if (SDL_GetError()[0] != '\0') {
		std::cerr << "[SDL] RenderCopy error renderer begin: " << SDL_GetError() << "\n";
		SDL_ClearError();
	}
	SDL_SetRenderScale(renderer, cam.zoom, cam.zoom);

	obj_container.render_all(renderer, cam);

	SDL_RenderPresent(renderer);
}

void Game::clean() {
	tex_mgr.clear();
	TTF_Quit();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	std::cout << "clean run" << std::endl;
}

bool Game::running() const {
	return run;
}