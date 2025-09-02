#include "tilemap.hpp"
using std::vector;

tilemap::tilemap(texture_manager* mgr, const std::string& tileset,
    int tileW, int tileH, int columns, int drawW, int drawH)
    : tex_mgr(mgr),
    tileset_name(tileset),
    tile_width(tileW), tile_height(tileH),
    draw_tile_width(drawW > 0 ? drawW : tileW),
    draw_tile_height(drawH > 0 ? drawH : tileH),
    tileset_columns(columns)
{
}

void tilemap::set_map(const vector<vector<int>>& map) {
    map_data = map;
    repeat_background = false;
    repeat_tile_index = -1;
}

void tilemap::fill_grid(int rows, int cols, int tile_index) {
    map_data.assign(rows, std::vector<int>(cols, tile_index));
    repeat_background = false;
    repeat_tile_index = -1;
}

void tilemap::fill_background(int /*screen_w*/, int /*screen_h*/, int tile_index) {
    repeat_background = true;
    repeat_tile_index = tile_index;
    map_data.clear();
}

inline SDL_FRect tilemap::src_from_index(int idx) const {
    const float sx = (idx % tileset_columns) * static_cast<float>(tile_width);
    const float sy = (idx / tileset_columns) * static_cast<float>(tile_height);
    return SDL_FRect{ sx, sy, (float)tile_width, (float)tile_height };
}

void tilemap::render(SDL_Renderer* renderer, const Camera& cam) {
    if (repeat_background && repeat_tile_index >= 0) {
        render_repeating_bg(renderer, cam, repeat_tile_index);
    }
    else {
        render_visible(renderer, cam);
    }
}

void tilemap::render_visible(SDL_Renderer* renderer, const Camera& cam) {
    SDL_Texture* tileset = tex_mgr->get_texture(tileset_name);
    if (!tileset || map_data.empty()) return;

    int outW = 0, outH = 0;
    SDL_GetCurrentRenderOutputSize(renderer, &outW, &outH);
    const float viewX = cam.x;
    const float viewY = cam.y;
    const float viewW = outW / cam.zoom;
    const float viewH = outH / cam.zoom;

    const int rows = (int)map_data.size();
    const int cols = rows ? (int)map_data[0].size() : 0;
    if (!rows || !cols) return;

    const int firstCol = std::max(0, (int)std::floor(viewX / draw_tile_width));
    const int firstRow = std::max(0, (int)std::floor(viewY / draw_tile_height));
    const int lastCol = std::min(cols - 1, (int)std::floor((viewX + viewW) / draw_tile_width));
    const int lastRow = std::min(rows - 1, (int)std::floor((viewY + viewH) / draw_tile_height));

    for (int r = firstRow; r <= lastRow; ++r) {
        const float dy = r * (float)draw_tile_height - cam.y;
        for (int c = firstCol; c <= lastCol; ++c) {
            const int idx = map_data[r][c];
            if (idx < 0) continue;

            const SDL_FRect src = src_from_index(idx);
            SDL_FRect dst{
                c * (float)draw_tile_width - cam.x,
                dy,
                (float)draw_tile_width,
                (float)draw_tile_height
            };
            SDL_RenderTexture(renderer, tileset, &src, &dst);
        }
    }
}

void tilemap::render_repeating_bg(SDL_Renderer* renderer, const Camera& cam, int tile_index) {
    SDL_Texture* tileset = tex_mgr->get_texture(tileset_name);
    if (!tileset) return;

    int outW = 0, outH = 0;
    SDL_GetCurrentRenderOutputSize(renderer, &outW, &outH);

    const float viewW = outW / cam.zoom;
    const float viewH = outH / cam.zoom;

    const SDL_FRect src = src_from_index(tile_index);

    auto positive_mod = [](float a, float m) {
        float r = std::fmod(a, m);
        if (r < 0) r += m;
        return r;
        };
    const float offsetX = -positive_mod(cam.x, (float)draw_tile_width);
    const float offsetY = -positive_mod(cam.y, (float)draw_tile_height);

    // Requires uniform scaling. If draw W/H are a uniform scale of source, use it:
    const float sx = (float)draw_tile_width / (float)tile_width;
    const float sy = (float)draw_tile_height / (float)tile_height;
    if (std::fabs(sx - sy) < 1e-6f) {
        SDL_FRect dst = {
            offsetX,
            offsetY,
            viewW + (float)draw_tile_width * 2.0f,
            viewH + (float)draw_tile_height * 2.0f
        };
        SDL_RenderTextureTiled(renderer, tileset, &src, sx, &dst);
        return;
    }
}
