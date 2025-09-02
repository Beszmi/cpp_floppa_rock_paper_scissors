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

	obj_container.spawn_as<streched_bg_obj>("-", "-", tex_mgr, 1.0f, true, -1);
	streched_bg_obj& floppa = *obj_container.get<streched_bg_obj>("-");
	floppa.init("floppa", tex_mgr, screen_w, screen_h);

	SDL_FPoint middle = ScreenPercentToWindow(renderer, 0.5f, 0.5f);
	tex_mgr.create_text_texture("cim", "fonts/ARIAL.TTF", 72, "FLOPPA ROCK PAPER SCISSORS", Colors::red);
	obj_container.spawn_as<Button>("cim", "cim", tex_mgr, middle.x - ((tex_mgr.get_texture("cim")->w / 2) * screen_scale_factor), (middle.y-(tex_mgr.get_texture("cim")->h / 2))/16, screen_scale_factor, true, 0);

	tex_mgr.create_text_texture("rock_text", "fonts/ARIAL.TTF", 48, "ROCK", Colors::white);
	tex_mgr.set_text_background("rock_text", true, Colors::light_grey, 4, 4);
	tex_mgr.set_text_border("rock_text", true, Colors::black, 2);
	obj_container.spawn_as<Text_Button>("rock_text", "rock_text", tex_mgr, middle.x - (middle.x / 2) - ((tex_mgr.get_texture("rock_test")->w / 2) * screen_scale_factor), (middle.y - (tex_mgr.get_texture("rock_test")->h / 2)) * 1.5, screen_scale_factor, true, 0, 0);

	tex_mgr.create_text_texture("paper_text", "fonts/ARIAL.TTF", 48, "PAPER", Colors::white);
	tex_mgr.set_text_background("paper_text", true, Colors::light_grey, 4, 4);
	tex_mgr.set_text_border("paper_text", true, Colors::black, 2);
	obj_container.spawn_as<Text_Button>("paper_test", "paper_text", tex_mgr, middle.x - ((tex_mgr.get_texture("paper_test")->w / 2) * screen_scale_factor), (middle.y - (tex_mgr.get_texture("paper_test")->h / 2)) * 1.5, screen_scale_factor, true, 0, 1);

	tex_mgr.create_text_texture("scissors_text", "fonts/ARIAL.TTF", 48, "SCISSORS", Colors::white);
	tex_mgr.set_text_background("scissors_text", true, Colors::light_grey, 4, 4);
	tex_mgr.set_text_border("scissors_text", true, Colors::black, 2);
	obj_container.spawn_as<Text_Button>("scissors_text", "scissors_text", tex_mgr, middle.x + (middle.x / 2) - ((tex_mgr.get_texture("scissors_test")->w / 2) * screen_scale_factor), (middle.y - (tex_mgr.get_texture("scissors_test")->h / 2)) * 1.5, screen_scale_factor, true, 0, 2);

	tex_mgr.create_text_texture("extra_text", "fonts/ARIAL.TTF", 48, "EXTRA BUTTON", Colors::white);
	tex_mgr.set_text_background("extra_text", true, Colors::light_grey, 4, 4);
	tex_mgr.set_text_border("extra_text", true, Colors::black, 2);
	obj_container.spawn_as<Text_Button>("extra_text", "extra_text", tex_mgr, middle.x, middle.y , screen_scale_factor, false, 1, 3);

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
					int result = hit->action();
					if (result > -1 && result < 3) {
						player1.add_stat(result);
						need_update = true;
						current_scene = 1;
					}
					if (result == 3) {
						player1.add_stat(result);
						need_update = true;
						current_scene = 0;
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
		if (current_scene == 1) {
			obj_container.layer_switch(1, true);
		} else if (current_scene == 0) {
			obj_container.layer_switch(1, false);
		}
		obj_container.rebuild_order();
		need_update = false;
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