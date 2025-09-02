#pragma once
#ifndef tilemap_h
#define tilemap_h

#include <vector>
#include <string>
#include <cmath>
#include <SDL3/SDL.h>
#include "texture_manager.hpp"
#include "camera.hpp"

class tilemap {
    int tile_width = 0, tile_height = 0;
    int draw_tile_width = 0, draw_tile_height = 0;
    int tileset_columns = 0;
    texture_manager* tex_mgr = nullptr;
    std::string tileset_name;

    std::vector<std::vector<int>> map_data;

    bool repeat_background = false;
    int  repeat_tile_index = -1;

public:
    tilemap(texture_manager* mgr,
        const std::string& tileset,
        int tileW, int tileH, int columns,
        int drawW = 0, int drawH = 0);

    void set_map(const std::vector<std::vector<int>>& map);
    void fill_grid(int rows, int cols, int tile_index);

    // Same name as before, but now it enables infinite, camera-anchored tiling at draw time.
    void fill_background(int /*screen_w*/, int /*screen_h*/, int tile_index);

    // Main render: draws visible map tiles or the repeating background.
    void render(SDL_Renderer* renderer, const Camera& cam);

private:
    inline SDL_FRect src_from_index(int idx) const;
    void render_visible(SDL_Renderer* renderer, const Camera& cam);
    void render_repeating_bg(SDL_Renderer* renderer, const Camera& cam, int tile_index);
};

#endif
