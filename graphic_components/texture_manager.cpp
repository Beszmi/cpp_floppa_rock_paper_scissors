#include "texture_manager.hpp"

namespace fs = std::filesystem;

texture_manager::texture_manager(SDL_Renderer* renderer) : renderer(renderer) {}

texture_manager::~texture_manager() { clear(); }

namespace {
    inline std::string to_lower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    inline std::string last_segment(const std::string& key) {
        const auto p = key.find_last_of('/');
        return (p == std::string::npos) ? key : key.substr(p + 1);
    }
}

auto texture_manager::find_iter_by_name(const std::string& name) -> decltype(textures)::iterator{
    auto it = textures.find(name);
    if (it != textures.end()) return it;

    if (name.find('/') == std::string::npos) {
        decltype(textures)::iterator found = textures.end();
        size_t hits = 0;
        for (auto iter = textures.begin(); iter != textures.end(); ++iter) {
            if (last_segment(iter->first) == name) {
                found = iter;
                ++hits;
                if (hits > 1) break;
            }
        }
        if (hits == 1) return found;
        if (hits > 1) {
            std::cerr << "Ambiguous texture name '" << name
                      << "' matches multiple keys. Use full path like 'folder/" << name << "'.\n";
        }
    }
    return textures.end();
}

//const version
auto texture_manager::find_iter_by_name(const std::string& name) const -> decltype(textures)::const_iterator{
    auto it = textures.find(name);
    if (it != textures.end()) return it;

    if (name.find('/') == std::string::npos) {
        decltype(textures)::const_iterator found = textures.end();
        size_t hits = 0;
        for (auto iter = textures.cbegin(); iter != textures.cend(); ++iter) {
            if (last_segment(iter->first) == name) {
                found = iter;
                ++hits;
                if (hits > 1) break;
            }
        }
        return (hits == 1) ? found : textures.end();
    }
    return textures.end();
}

SDL_Texture* texture_manager::load_texture(const std::string& name, const std::string& filename) {
    if (textures.find(name) != textures.end()) {
        return textures[name];
    }

    SDL_Surface* surface = IMG_Load(filename.c_str());
    if (!surface) {
        std::cerr << "Failed to load image: " << filename << "\n";
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    if (!texture) {
        std::cerr << "Failed to create texture from: " << filename << "\n";
        std::cerr << "SDL error: " << SDL_GetError() << "\n";
        return nullptr;
    }

    textures[name] = texture;
    return texture;
}

SDL_Texture* texture_manager::get_texture(const std::string& name) const {
    auto it = textures.find(name);
    return (it != textures.end()) ? it->second : nullptr;
}

void texture_manager::load_textures_from_folder(const std::string& folder_path) {
    const fs::path base = folder_path;

    std::error_code ec;
    for (fs::recursive_directory_iterator it(base, ec), end; it != end; it.increment(ec)) {
        if (ec) {
            std::cerr << "Filesystem iteration error: " << ec.message() << "\n";
            break;
        }

        const fs::directory_entry& entry = *it;
        if (!entry.is_regular_file()) continue;

        const fs::path& p = entry.path();
        std::string ext = to_lower(p.extension().string());
        if (ext != ".png" && ext != ".jpg" && ext != ".jpeg") continue;

        // key = relative_dir / stem  (no extension), using '/' separators
        std::error_code ec2;
        fs::path rel_dir = fs::relative(p.parent_path(), base, ec2);
        if (ec2) rel_dir.clear();

        fs::path key_path = (rel_dir.empty() || rel_dir == ".") ? p.stem() : (rel_dir / p.stem());
        std::string key = key_path.generic_string(); // '/' on all platforms

        load_texture(key, p.string());
    }
}

void texture_manager::unload_texture(const std::string& name) {
    auto it = textures.find(name);
    if (it != textures.end()) {
        SDL_DestroyTexture(it->second);
        textures.erase(it);
    }
}

bool texture_manager::has(const string& name) {
    return textures.find(name) != textures.end();
}

void texture_manager::clear() {
    for (auto& [name, texture] : textures) {
        SDL_DestroyTexture(texture);
    }
    textures.clear();
    text_meta.clear();

    for (auto& [key, font] : fonts) {
        TTF_CloseFont(font);
    }
    fonts.clear();
}

std::string texture_manager::font_key(const std::string& family, float pt) {
    char buf[64]; SDL_snprintf(buf, sizeof(buf), "@%.2f", pt);
    return family + buf;
}

TTF_Font* texture_manager::get_or_load_font(const std::string& family, float pt) {
    auto key = font_key(family, pt);
    if (auto it = fonts.find(key); it != fonts.end()) return it->second;
    return load_font(family, pt);
}

TTF_Font* texture_manager::load_font(const std::string& family, float pt) {
    auto key = font_key(family, pt);
    if (fonts.count(key)) return fonts[key];

    TTF_Font* f = TTF_OpenFont(family.c_str(), pt);
    if (!f) {
        SDL_Log("TTF_OpenFont failed (%s @ %.2fpt): %s", family.c_str(), pt, SDL_GetError());
        return nullptr;
    }
    fonts[key] = f;
    return f;
}

void texture_manager::unload_font(const std::string& family, float pt) {
    auto key = font_key(family, pt);
    if (auto it = fonts.find(key); it != fonts.end()) {
        TTF_CloseFont(it->second);
        fonts.erase(it);
    }
}
bool texture_manager::has_font(const std::string& family, float pt) const {
    return fonts.count(font_key(family, pt)) != 0;
}

// Build or rebuild the SDL_Texture for a TextEntry
bool texture_manager::rerender_text_texture(TextEntry& e) {
    TTF_Font* font = get_or_load_font(e.family, e.ptsize);
    if (!font) return false;

    SDL_Surface* text = (e.wrap_width > 0)
        ? TTF_RenderText_Blended_Wrapped(font, e.text.c_str(), e.text.size(), e.color, e.wrap_width)
        : TTF_RenderText_Blended(font, e.text.c_str(), e.text.size(), e.color);
    if (!text) {
        SDL_Log("TTF_RenderText failed: %s", SDL_GetError());
        return false;
    }

    // --- compute outer size ---
    const int bw = e.border_enabled ? e.border_thickness : 0;
    const int padw = e.pad_x, padh = e.pad_y;
    const int inner_w = text->w + 2*padw;
    const int inner_h = text->h + 2*padh;
    const int out_w   = inner_w + 2*bw;
    const int out_h   = inner_h + 2*bw;

    SDL_Surface* out = SDL_CreateSurface(out_w, out_h, SDL_PIXELFORMAT_RGBA32);
    if (!out) {
        SDL_DestroySurface(text);
        SDL_Log("SDL_CreateSurface failed: %s", SDL_GetError());
        return false;
    }

    // Start fully transparent
    Uint32 transparent = SDL_MapSurfaceRGBA(out, 0,0,0,0);
    SDL_FillSurfaceRect(out, nullptr, transparent);

    // Background (if enabled)
    if (e.bg_enabled && e.bg_color.a > 0) {
        SDL_Rect bg{ bw, bw, inner_w, inner_h };
        Uint32 bgpx = SDL_MapSurfaceRGBA(out, e.bg_color.r, e.bg_color.g, e.bg_color.b, e.bg_color.a);
        SDL_FillSurfaceRect(out, &bg, bgpx);
    }

    // Border (if enabled)
    if (e.border_enabled && bw > 0 && e.border_color.a > 0) {
        Uint32 bpx = SDL_MapSurfaceRGBA(out, e.border_color.r, e.border_color.g, e.border_color.b, e.border_color.a);
        SDL_Rect top   { 0,           0,        out_w, bw };
        SDL_Rect bottom{ 0,  out_h - bw,        out_w, bw };
        SDL_Rect left  { 0,           0,        bw,    out_h };
        SDL_Rect right { out_w - bw,  0,        bw,    out_h };
        SDL_FillSurfaceRect(out, &top,    bpx);
        SDL_FillSurfaceRect(out, &bottom, bpx);
        SDL_FillSurfaceRect(out, &left,   bpx);
        SDL_FillSurfaceRect(out, &right,  bpx);
    }
    SDL_Rect dst{ bw + padw, bw + padh, text->w, text->h };
    SDL_BlitSurface(text, nullptr, out, &dst);

    // Replace existing texture
    if (auto it = textures.find(e.name); it != textures.end() && it->second) {
        SDL_DestroyTexture(it->second);
    }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, out);
    SDL_DestroySurface(text);
    SDL_DestroySurface(out);
    if (!tex) {
        SDL_Log("CreateTextureFromSurface failed: %s", SDL_GetError());
        return false;
    }
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    textures[e.name] = tex;
    return true;
}


SDL_Texture* texture_manager::create_text_texture(const std::string& name, const std::string& family, float ptsize, const std::string& text, const SDL_Color& color, const SDL_Color& bg_color,  int wrap_width, const std::string& quality) {
    TextEntry meta;
    meta.name = name;
    meta.family = family;
    meta.ptsize = ptsize;
    meta.text = text;
    meta.color = color;
    if (bg_color.a > 0) {
        meta.bg_enabled = true;
        meta.bg_color = bg_color;
    }
    meta.wrap_width = wrap_width;
    meta.quality = quality;

    if (!rerender_text_texture(meta)) return nullptr;
    text_meta[name] = std::move(meta);
    return textures[name];
}


bool texture_manager::set_text_string(const std::string& name, const std::string& new_text) {
    auto it = text_meta.find(name); if (it == text_meta.end()) return false;
    if (it->second.text == new_text) return true; //skip
    it->second.text = new_text;
    return rerender_text_texture(it->second);
}

bool texture_manager::set_text_color(const std::string& name, SDL_Color new_color) {
    auto it = text_meta.find(name); if (it == text_meta.end()) return false;
    if (!std::memcmp(&it->second.color, &new_color, sizeof(SDL_Color))) return true;
    it->second.color = new_color;
    return rerender_text_texture(it->second);
}

bool texture_manager::set_text_size(const std::string& name, float new_ptsize) {
    auto it = text_meta.find(name); if (it == text_meta.end()) return false;
    if (it->second.ptsize == new_ptsize) return true;
    it->second.ptsize = new_ptsize;
    return rerender_text_texture(it->second);
}

bool texture_manager::set_text_background(const std::string& name, bool enabled, SDL_Color color, int pad_x, int pad_y) {
    auto it = text_meta.find(name); if (it == text_meta.end()) return false;
    it->second.bg_enabled = enabled;
    if (enabled) {
        it->second.bg_color = color;
        it->second.pad_x = pad_x;
        it->second.pad_y = pad_y;
    }
    return rerender_text_texture(it->second);
}

bool texture_manager::set_text_wrap(const std::string& name, int new_wrap) {
    auto it = text_meta.find(name); if (it == text_meta.end()) return false;
    if (it->second.wrap_width == new_wrap) return true;
    it->second.wrap_width = new_wrap;
    return rerender_text_texture(it->second);
}

bool texture_manager::set_text_border(const std::string& name, bool enabled, SDL_Color color, int thickness) {
    auto it = text_meta.find(name); if (it == text_meta.end()) return false;
    it->second.border_enabled = enabled;
    if (enabled) {
        it->second.border_color = color;
        it->second.border_thickness = thickness;
    }
    return rerender_text_texture(it->second);
}
