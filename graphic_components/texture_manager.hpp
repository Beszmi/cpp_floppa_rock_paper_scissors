#pragma once
#ifndef texture_manager_h
#define texture_manager_h
#include <iostream>
#include <unordered_map>
#include <string>
#include <filesystem>
#include <algorithm>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

using namespace std;

class texture_manager {
	unordered_map<string, SDL_Texture*> textures;
    unordered_map<std::string, TTF_Font*> fonts;
	SDL_Renderer* renderer;
    SDL_Texture* missing_texture = nullptr;
    struct TextEntry {
        std::string name;
        std::string family;
        float       ptsize = 16;
        SDL_Color   color{ 255,255,255,255 };
        int         wrap_width = 0;
        std::string text;
        std::string quality = "blended";
        bool        bg_enabled = false;
        SDL_Color   bg_color{ 0,0,0,0 };
        bool        border_enabled = false;
        SDL_Color   border_color{ 0,0,0,0 };
        int         border_thickness = 0;
        int         pad_x = 0, pad_y = 0;
    };
    std::unordered_map<std::string, TextEntry> text_meta;

    static std::string font_key(const std::string& family, float pt);
    TTF_Font* get_or_load_font(const std::string& family, float pt);
    bool rerender_text_texture(TextEntry& e);
public:
    texture_manager(SDL_Renderer* renderer);
    ~texture_manager();

    auto find_iter_by_name(const std::string& name) -> decltype(textures)::iterator;
    auto find_iter_by_name(const std::string& name) const -> decltype(textures)::const_iterator;
    SDL_Texture* load_texture(const string& name, const string& filename);
    SDL_Texture* get_texture(const string& name) const;
    void load_textures_from_folder(const string& folder_path);
    void set_renderer(SDL_Renderer* rend) { renderer = rend; }
    void unload_texture(const string& name);
    bool has(const string& name);
    void clear();

    //text
    TTF_Font* load_font(const std::string& family, float pt);
    void        unload_font(const std::string& family, float pt);
    bool        has_font(const std::string& family, float pt) const;

    SDL_Texture* create_text_texture(const std::string& name, const std::string& family, float ptsize, const std::string& text, const SDL_Color& color, const SDL_Color& bg_color = {0,0,0,0}, int wrap_width = 0, const std::string& quality = "blended");

    bool set_text_string(const std::string& name, const std::string& new_text);
    bool set_text_color(const std::string& name, SDL_Color new_color);
    bool set_text_size(const std::string& name, float new_ptsize);
    bool set_text_wrap(const std::string& name, int new_wrap);
    bool set_text_background(const std::string& name, bool enabled, SDL_Color color = { 0,0,0,0 }, int pad_x = 0, int pad_y = 0);
    bool set_text_background_const_padding(const std::string& name, bool enabled, SDL_Color color = { 0,0,0,0 });
    bool set_text_border(const std::string& name, bool enabled, SDL_Color color = { 0,0,0,0 }, int thickness = 1);
    SDL_Color& get_bg_color(const std::string& name);
};

#endif