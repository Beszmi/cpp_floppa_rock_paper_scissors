#pragma once

#ifndef game_obj_hpp
#define game_obj_hpp
#include <iostream>
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "graphic_components/texture_manager.hpp"
#include "graphic_components/camera.hpp"
#include "graphic_components/sprites.hpp"
#include <cmath>
#include "text.hpp"

//game objects

class GameObject_cluster; // for ctors

class GameObject {
	std::string name;
	Transform transform;
	SDL_Texture* obj_tex;
	SDL_FRect src_rect;
	SDL_FRect dst_rect;
	bool show;
	float scale;
	int layer;
protected:
	bool active = false;
	bool hover = false;
	void set_texture(SDL_Texture* t) { obj_tex = t; }
public:
	GameObject(const std::string& name, const std::string& texture, const texture_manager& tex_mgr, float scale = 1.0f, bool show_it = false, int layer_in = 0);
	GameObject(const std::string& name, const std::string& texture, const texture_manager& tex_mgr, int x, int y, float scale = 1.0f, bool show_it = false, int layer_in = 0);
	GameObject(const std::string& name, const std::string& texture, const texture_manager& tex_mgr, GameObject_cluster* prn, float scale = 1.0f, bool show_it = false, int layer_in = 0);
	GameObject(const std::string& name, const std::string& texture, const texture_manager& tex_mgr, int x, int y, GameObject_cluster* prn, float scale = 1.0f, bool show_it = false, int layer_in = 0);
	GameObject(const GameObject& rhs);

	const std::string& get_name() const { return name; }
	double get_world_x() const { return transform.worldX; }
	double get_world_y() const { return transform.worldY; }
	double get_loc_x() const { return transform.localX; }
	double get_loc_y() const { return transform.localY; }
	Transform* get_transform() { return &transform; }
	const Transform* get_transform() const { return &transform; }

	void set_loc_position(double x, double y) { transform.setLocal(x, y); }
	void set_loc_x(double x) { transform.localX = x; transform.dirty = true; }
	void set_loc_y(double y) { transform.localY = y; transform.dirty = true; }

	virtual void update(double dt, double speed = 400);
	virtual void render(SDL_Renderer* ren, const Camera& cam) const;
	virtual ~GameObject() = default;
	virtual void action() { std::cout << "my name " << name << std::endl; };
	virtual std::unique_ptr<GameObject> clone() const;

	SDL_Texture* get_tex();
	SDL_Texture* get_tex() const;
	SDL_FRect& get_src_rect() { return src_rect; }
	SDL_FRect& get_dst_rect() { return dst_rect; }
	const SDL_FRect& get_src_rect() const { return src_rect; }
	const SDL_FRect& get_dst_rect() const { return dst_rect; }
	void set_dst_rect(double x, double y);

	float get_scale() const { return scale; }
	void set_scale(float scale_in) { scale = scale_in; }

	bool does_show() const { return show; }
	void set_show(bool v) { show = v; }

	int  get_layer() const { return layer; }
	void set_layer(int l) { layer = l; } // call rebuild_order in the container after this

	virtual bool hit_test(float wx, float wy) const;

	virtual void on_hover_enter(SDL_Cursor* pointer_cursor) { hover = true; }
	virtual void on_hover() {}
	virtual void on_hover_exit(SDL_Cursor* default_cursor) { hover = false; }
	virtual void on_hold_start(int button) {}
	virtual void on_hold(double seconds, int button) {}
	virtual void on_hold_end(double seconds, int button, bool canceled) {}
};

class Game_obj_container {
	std::unordered_map<std::string, std::unique_ptr<GameObject>> objects;

	mutable std::vector<GameObject*> render_order_;
	mutable bool order_dirty = true;
public:

	void rebuild_order() const;
	void set_layer(GameObject& obj, int new_layer);
	void invalidate_render_order() { order_dirty = true; } //call after direct change to game_obj's state, otherwise set_layer handles it

	template<typename T, typename... Args>
	T* spawn_as(const std::string& name, Args&&... args) {
		auto p = std::make_unique<T>(name, std::forward<Args>(args)...);
		T* raw = p.get();
		objects[name] = std::move(p);
		return raw;
	}

	template<class T = GameObject>
	T* get(const std::string& name) {
		static_assert(std::is_base_of_v<GameObject, T>,
			"T must derive from GameObject");
		auto it = objects.find(name);
		if (it == objects.end()) return nullptr;
		return dynamic_cast<T*>(it->second.get());
	}

	template<class T = GameObject>
	const T* get(const std::string& name) const {
		static_assert(std::is_base_of_v<GameObject, T>,
			"T must derive from GameObject");
		auto it = objects.find(name);
		if (it == objects.end()) return nullptr;
		return dynamic_cast<const T*>(it->second.get());
	}
	void update_all(double dtSeconds, double speed = 400);
	void render_all(SDL_Renderer* ren, const Camera& cam) const;
	void set_scale_all(float new_scale);

	GameObject* pick_topmost(float wx, float wy) const;
};

//Game object cluster -----------------------------------------------------------------------------------

class GameObject_cluster: public GameObject {
	std::vector<std::unique_ptr<GameObject>> items;
public:
	using GameObject::GameObject;

	//void action()  override { std::cout << "test"; }

	void add_item_local(const GameObject& obj_in, int lx, int ly, bool show_in_clust = false);
	void add_item_world(const GameObject& obj_in, bool show_in_clust = false);

	void update(double dt, double speed = 400) override;
	void render(SDL_Renderer* ren, const Camera& cam) const override;
	~GameObject_cluster() = default;
};

// DERIVED OBJECTS --------------------------------------------------------------------------------------

class Button : public GameObject {
public:
	using GameObject::GameObject;
	//void action()  override { std::cout << "test"; }
	std::unique_ptr<GameObject> clone() const override { return std::make_unique<Button>(*this); }

	void update(double dt, double speed = 400) override;
	using GameObject::render;
	void on_hover_enter(SDL_Cursor* pointer_cursor) override;
	void on_hover_exit(SDL_Cursor* default_cursor) override;

	~Button() = default;
};

class Text_Button : public GameObject {
	texture_manager& tex_mgr;
public:
	Text_Button(const std::string& name, const std::string& texture, texture_manager& tex_mgr_in, float scale = 1.0f, bool show_it = false, int layer_in = 0);
	Text_Button(const std::string& name, const std::string& texture, texture_manager& tex_mgr_in, int x, int y, float scale = 1.0f, bool show_it = false, int layer_in = 0);
	Text_Button(const std::string& name, const std::string& texture, texture_manager& tex_mgr_in, GameObject_cluster* prn, float scale = 1.0f, bool show_it = false, int layer_in = 0);
	Text_Button(const std::string& name, const std::string& texture, texture_manager& tex_mgr_in, int x, int y, GameObject_cluster* prn, float scale = 1.0f, bool show_it = false, int layer_in = 0);
	//void action()  override { std::cout << "test"; }
	std::unique_ptr<GameObject> clone() const override { return std::make_unique<Text_Button>(*this); }

	void update(double dt, double speed = 400) override;
	using GameObject::render;
	void on_hover_enter(SDL_Cursor* pointer_cursor) override;
	void on_hover_exit(SDL_Cursor* default_cursor) override;

	~Text_Button() = default;
};

//streched bg obj

class streched_bg_obj : public GameObject {
	strech_bg image;
public:
	using GameObject::GameObject;
	void set_texture(const std::string& texture, const texture_manager& tex_mgr);
	void set_screen(int screen_w, int screen_h);
	void init(const std::string& texture, const texture_manager& tex_mgr, int screen_w, int screen_h);

	void update(double dt, double speed = 400) override;

	void render(SDL_Renderer* ren, const Camera& cam) const override;

	~streched_bg_obj() = default;
};

//sprite object

class sprite : public GameObject {
	std::vector<std::unique_ptr<sprite_component>> elements;
	int state = -4; //states in docs
	int current_element = 0;
	double t = 0;
	double tick_time = 0.25;
public:
	using GameObject::GameObject;

	void add_element(const std::string& texture, const texture_manager& tex_mgr);
	//void add_elements_batch(const std::string& name);
	std::unique_ptr<sprite_component>& get_element(size_t idx);

	int get_state() const { return state; }
	void set_state(int new_State) { state = new_State; }

	size_t get_current_idx() const { return static_cast<size_t>(current_element); }
	void set_current_idx(int idx);

	void update(double dt, double speed = 1) override;

	void render(SDL_Renderer* ren, const Camera& cam) const override;

	void action() override;
};

#endif