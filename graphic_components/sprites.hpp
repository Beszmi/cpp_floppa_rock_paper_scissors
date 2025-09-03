#pragma once
#ifndef sprites_hpp
#define sprites_hpp
#include <memory>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "texture_manager.hpp"
#include "camera.hpp"

class sprite_component {
	SDL_Texture* obj_tex;
public:
	sprite_component();
	sprite_component(const std::string& texture, const texture_manager& tex_mgr);
	virtual void render(SDL_Renderer* ren, const SDL_FRect* src_rect, const SDL_FRect* dst_rect, const Camera& cam) const;
	virtual void render(SDL_Renderer* ren, const SDL_FRect* src_rect, const SDL_FRect* dst_rect, const Camera& cam, double scale) const;
	SDL_Texture* get_tex() { return obj_tex; }
	void set_tex(const std::string& texture, const texture_manager& tex_mgr);
	virtual ~sprite_component() {}
};

class strech_bg :public sprite_component {
	SDL_FRect src_rect;
	SDL_FRect screen;
public:
	strech_bg();
	strech_bg(const std::string& texture, const texture_manager& tex_mgr, int screen_w, int screen_h);
	void set_screen(int screen_w, int screen_h);
	void render(SDL_Renderer* ren, const Camera& cam) const;
	using sprite_component::get_tex;
	using sprite_component::set_tex;
	~strech_bg() {}
};

#endif