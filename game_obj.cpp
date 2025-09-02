#include "game_obj.hpp"

//BASE OBJECT
GameObject::GameObject(const std::string& name, const std::string& texture, const texture_manager& tex_mgr, float scale, bool show_it, int layer_in): name(name), scale(scale), show(show_it), obj_tex(nullptr), layer(layer_in) {
	if (texture != "-") {
		obj_tex = tex_mgr.get_texture(texture);
		if (!obj_tex) {
			std::cerr << "Texture not found for '" << texture << "'\n";
		}
	}
	set_loc_position(0, 0);
	transform.setWorld(0, 0);
	float w = 0, h = 0;
	if (obj_tex) SDL_GetTextureSize(obj_tex, &w, &h);
	src_rect = { 0, 0, w, h };
	dst_rect = { 0, 0, w, h };
}

GameObject::GameObject(const std::string& name, const std::string& texture, const texture_manager& tex_mgr, int x, int y, float scale, bool show_it, int layer_in) : name(name), scale(scale), show(show_it), obj_tex(nullptr), layer(layer_in) {
	if (texture != "-") {
		obj_tex = tex_mgr.get_texture(texture);
		if (!obj_tex) {
			std::cerr << "Texture not found for '" << texture << "'\n";
		}
	}
	set_loc_position(x, y);
	transform.setWorld(x, y);
	float w = 0, h = 0;
	if (obj_tex) SDL_GetTextureSize(obj_tex, &w, &h);
	src_rect = { 0, 0, w, h };
	dst_rect = { (float)round(transform.worldX), (float)round(transform.worldY), w, h };
}

GameObject::GameObject(const std::string& name, const std::string& texture, const texture_manager& tex_mgr, GameObject_cluster* prn, float scale, bool show_it, int layer_in): name(name), scale(scale), show(show_it), obj_tex(nullptr), layer(layer_in) {
	if (texture != "-") {
		obj_tex = tex_mgr.get_texture(texture);
		if (!obj_tex) {
			std::cerr << "Texture not found for '" << texture << "'\n";
		}
	}
	transform.parent = prn->get_transform();
	set_loc_position(0, 0);

	float w = 0, h = 0;
	if (obj_tex) SDL_GetTextureSize(obj_tex, &w, &h);
	transform.computeWorld();
	src_rect = { 0, 0, w, h };
	dst_rect = { (float)round(transform.worldX), (float)round(transform.worldY), w, h };
}

GameObject::GameObject(const std::string& name, const std::string& texture, const texture_manager& tex_mgr, int x, int y, GameObject_cluster* prn, float scale, bool show_it, int layer_in): name(name), scale(scale), show(show_it), obj_tex(nullptr), layer(layer_in) {
	if (texture != "-") {
		obj_tex = tex_mgr.get_texture(texture);
		if (!obj_tex) {
			std::cerr << "Texture not found for '" << texture << "'\n";
		}
	}
	transform.parent = prn->get_transform();
	set_loc_position(x, y);

	float w = 0, h = 0;
	if (obj_tex) SDL_GetTextureSize(obj_tex, &w, &h);
	transform.computeWorld();
	src_rect = { 0, 0, w, h };
	dst_rect = { (float)round(transform.worldX), (float)round(transform.worldY), w, h};
}

GameObject::GameObject(const GameObject& rhs) : name(rhs.name), obj_tex(rhs.obj_tex), transform(rhs.transform), src_rect(rhs.src_rect), dst_rect(rhs.dst_rect), scale(rhs.scale), show(rhs.show), layer(rhs.layer) {}

void GameObject::set_dst_rect(double x, double y) {
	dst_rect.x = static_cast<float>(std::lround(x));
	dst_rect.y = static_cast<float>(std::lround(y));
}

void GameObject::update(double dt, double speed) {
	set_loc_position(transform.localX + (speed * dt), transform.localY + (speed * dt));
	transform.computeWorld();
	dst_rect.x = static_cast<float>(std::lround(transform.worldX));
	dst_rect.y = static_cast<float>(std::lround(transform.worldY));
}

void GameObject::render(SDL_Renderer* ren, const Camera& cam) const {
	if (!show || !obj_tex) return;
	SDL_FRect w = dst_rect;
	w.w *= scale;
	w.h *= scale;

	SDL_FRect r = WorldToRender(w, cam);

	SDL_RenderTexture(ren, obj_tex, &src_rect, &r);
}

std::unique_ptr<GameObject> GameObject::clone() const {
	return std::make_unique<GameObject>(*this);
}

SDL_Texture* GameObject::get_tex() {
	return obj_tex;
}

SDL_Texture* GameObject::get_tex() const {
	return obj_tex;
}

bool GameObject::hit_test(float wx, float wy) const {
	SDL_FRect b = dst_rect; b.w *= scale; b.h *= scale;
	return (wx >= b.x && wx < b.x + b.w && wy >= b.y && wy < b.y + b.h);
}


//CONTAINER

void Game_obj_container::rebuild_order() const {
	render_order_.clear();
	render_order_.reserve(objects.size());
	for (auto& [_, up] : objects) {
		if (up->does_show()) render_order_.push_back(up.get());
	}
	std::stable_sort(render_order_.begin(), render_order_.end(),
		[](const GameObject* a, const GameObject* b) {
			if (a->get_layer() != b->get_layer())
				return a->get_layer() < b->get_layer();
			return std::less<const GameObject*>{}(a, b);
		});
}

void Game_obj_container::set_layer(GameObject& obj, int new_layer) {
	if (obj.get_layer() != new_layer) {
		obj.set_layer(new_layer);
		order_dirty = true;
	}
}

void Game_obj_container::update_all(double dtSeconds, double speed) {
	for (auto& [_, obj] : objects) {
		obj->update(dtSeconds, speed);
	}
}

void Game_obj_container::render_all(SDL_Renderer* ren, const Camera& cam) const {
	if (order_dirty) {
		rebuild_order();
		order_dirty = false;
	}
	SDL_SetRenderScale(ren, cam.zoom, cam.zoom);

	int outW = 0, outH = 0;
	SDL_GetCurrentRenderOutputSize(ren, &outW, &outH);
	SDL_FRect worldView = { cam.x, cam.y, outW / cam.zoom, outH / cam.zoom };

	auto intersects = [](const SDL_FRect& a, const SDL_FRect& b) {
		return !(a.x > b.x + b.w || a.x + a.w < b.x || a.y > b.y + b.h || a.y + a.h < b.y);
		};

	for (const GameObject* o : render_order_) {
		if (intersects(o->get_dst_rect(), worldView)) {
			o->render(ren, cam);
		}
	}
}

void Game_obj_container::set_scale_all(float new_scale) {
	for (auto& [_, obj] : objects) {
		obj->set_scale(new_scale);
	}
}

GameObject* Game_obj_container::pick_topmost(float wx, float wy) const {
	if (order_dirty) { rebuild_order(); order_dirty = false; }
	for (auto it = render_order_.rbegin(); it != render_order_.rend(); ++it) {
		GameObject* o = *it;
		if (o->does_show() && o->hit_test(wx, wy)) return o;
	}
	return nullptr;
}

//OBJECT CLUSTER

void GameObject_cluster::add_item_local(const GameObject& obj_in, int lx, int ly, bool show_in_clust) {
	auto p = obj_in.clone();
	p->get_transform()->parent = get_transform();
	p->get_transform()->setLocal((float)lx, (float)ly);
	p->set_show(show_in_clust);
	items.push_back(std::move(p));
	get_transform()->markDirty();
}

void GameObject_cluster::add_item_world(const GameObject& obj_in, bool show_in_clust) {
	auto p = obj_in.clone();
	p->get_transform()->parent = get_transform();
	this->get_transform()->computeWorld();
	double lx = obj_in.get_transform()->worldX - this->get_transform()->worldX;
	double ly = obj_in.get_transform()->worldY - this->get_transform()->worldY;
	p->get_transform()->setLocal(lx, ly);
	p->set_show(show_in_clust);
	items.push_back(std::move(p));
}

void GameObject_cluster::update(double dt, double speed) {
	speed = 150;
	get_transform()->computeWorld();
	GameObject::update(dt, speed);

	for (auto& c : items) {
		c->update(dt, 0);
	}
}

void GameObject_cluster::render(SDL_Renderer* ren, const Camera& cam) const {
	//render self
	GameObject::render(ren, cam);
	if (SDL_GetError()[0] != '\0') {
		SDL_Log("[SDL] RenderCopy (cluster) error: %s", SDL_GetError());
		SDL_ClearError();
	}

	//render owned
	for (auto& c : items) {
		c->render(ren, cam);
	}
}

// BUTTONS

void Button::update(double dt, double speed) {
	GameObject::update(0.0, 0.0);
}

void Button::on_hover_enter(SDL_Cursor* pointer_cursor) {
	SDL_SetCursor(pointer_cursor);
}

void Button::on_hover_exit(SDL_Cursor* default_cursor) {
	SDL_SetCursor(default_cursor);
}

// TEXT BUTTONS

Text_Button::Text_Button(const std::string& name, const std::string& texture, texture_manager& tex_mgr_in, float scale, bool show_it, int layer_in) 
	: GameObject(name, texture, tex_mgr_in, scale, show_it, layer_in), tex_mgr(tex_mgr_in) {}

Text_Button::Text_Button(const std::string& name, const std::string& texture, texture_manager& tex_mgr_in, int x, int y, float scale, bool show_it, int layer_in)
	: GameObject(name, texture, tex_mgr_in, x, y, scale, show_it, layer_in), tex_mgr(tex_mgr_in) {}

Text_Button::Text_Button(const std::string& name, const std::string& texture, texture_manager& tex_mgr_in, GameObject_cluster* prn, float scale, bool show_it, int layer_in) 
	: GameObject(name, texture, tex_mgr_in, prn, scale, show_it, layer_in), tex_mgr(tex_mgr_in) {}

Text_Button::Text_Button(const std::string& name, const std::string& texture, texture_manager& tex_mgr_in, int x, int y, GameObject_cluster* prn, float scale, bool show_it, int layer_in)
	: GameObject(name, texture, tex_mgr_in, x, y, prn, scale, show_it, layer_in), tex_mgr(tex_mgr_in) {}

void Text_Button::update(double dt, double speed) {
	GameObject::update(0.0, 0.0);
}

void Text_Button::on_hover_enter(SDL_Cursor* pointer_cursor) {
	hover = true;
	SDL_SetCursor(pointer_cursor);

	if (tex_mgr.set_text_background_const_padding(get_name(), true, Colors::grey)) {
		if (auto* t = tex_mgr.get_texture(get_name())) {
			set_texture(t);
			float w = 0, h = 0; SDL_GetTextureSize(t, &w, &h);
			get_src_rect() = { 0,0,w,h };
			auto& d = get_dst_rect(); d.w = w; d.h = h;
		}
	}
}

void Text_Button::on_hover_exit(SDL_Cursor* default_cursor) {
	hover = false;
	SDL_SetCursor(default_cursor);

	if (tex_mgr.set_text_background_const_padding(get_name(), true, Colors::light_grey)) {
		if (auto* t = tex_mgr.get_texture(get_name())) {
			set_texture(t);
			float w = 0, h = 0; SDL_GetTextureSize(t, &w, &h);
			get_src_rect() = { 0,0,w,h };
			auto& d = get_dst_rect(); d.w = w; d.h = h;
		}
	}
}

//sprite objs

void streched_bg_obj::set_texture(const std::string& texture, const texture_manager& tex_mgr) {
	image.set_tex(texture, tex_mgr);
}

void streched_bg_obj::set_screen(int screen_w, int screen_h) {
	image.set_screen(screen_w, screen_h);
}

void streched_bg_obj::init(const std::string& texture, const texture_manager& tex_mgr, int screen_w, int screen_h) {
	image.set_tex(texture, tex_mgr);
	image.set_screen(screen_w, screen_h);
}

void streched_bg_obj::update(double dt, double speed) {
	GameObject::update(0.0, 0.0);
}

void streched_bg_obj::render(SDL_Renderer* ren, const Camera& cam) const {
	if (does_show()) {
		image.render(ren, cam);
	}
}

//sprite ------------------------------------------------------------

void sprite::add_element(const std::string& texture, const texture_manager& tex_mgr) {
	if (elements.size() < 1) {
		float w, h;
		auto adding_Tex = tex_mgr.get_texture(texture);
		SDL_GetTextureSize(adding_Tex, &w, &h);
		get_src_rect() = { 0, 0, w, h };
		get_dst_rect() = { (float)round(get_transform()->worldX), (float)round(get_transform()->worldY), w, h };
	}

	auto p = std::make_unique<sprite_component>(texture, tex_mgr);
	elements.push_back(std::move(p));
}

//void sprite::add_elements_batch(const std::string& name) {}

std::unique_ptr<sprite_component>& sprite::get_element(size_t idx) {
	return elements.at(idx);
}

void sprite::set_current_idx(int idx) {
	if (idx < 0) idx = static_cast<int>(elements.size()) - 1;
	if (idx >= static_cast<int>(elements.size())) idx = 0;
	current_element = idx;
}
 

void sprite::update(double dt, double speed) {
	switch (state){
		case 0:
			GameObject::update(0.0, 0.0);
			break;
		case 1:
			t = t + dt;
			if (t >= tick_time) {
				set_current_idx(++current_element);
				GameObject::update(0.0, 0.0);
				t = 0;
			}
			break;
		case -1:
			t = t + dt;
			if (t >= tick_time) {
				set_current_idx(current_element - 1);
				GameObject::update(0.0, 0.0);
				t = 0;
			}
			break;
		case 2:
			if (active) {
				set_current_idx(++current_element);
				GameObject::update(0.0, 0.0);
				active = false;
			}
			break;
		case -2:
			if (active) {
				set_current_idx(current_element - 1);
				GameObject::update(0.0, 0.0);
				active = false;
			}
			break;
		case 4:
			t = t + dt;
			if (t >= tick_time) {
				if (hover) {
					set_current_idx(++current_element);
					GameObject::update(0.0, 0.0);
					t = 0;
				}
			}
			break;
		case -4:
			t = t + dt;
			if (t >= tick_time) {
				if (hover) {
					set_current_idx(current_element - 1);
					GameObject::update(0.0, 0.0);
					t = 0;
				}
			}
			break;
		default:
			GameObject::update(0.0, 0.0);
			break;
		}
}

void sprite::render(SDL_Renderer* ren, const Camera& cam) const {
	if (does_show()) {
		if (current_element >= elements.size()) {
			std::cout << "sprite currently rendering id too large" << std::endl;
		}
		else {
			elements.at(current_element)->render(ren, &get_src_rect(), &get_dst_rect(), cam, 1.0f);
		}
	}
}

void sprite::action() {
	active = true;
}