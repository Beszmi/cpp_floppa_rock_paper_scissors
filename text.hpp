#pragma once
#ifndef text_hpp
#define text_hpp
#include <iostream>
#include <memory>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

namespace Colors {
    constexpr SDL_Color rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        return SDL_Color{ r, g, b, a };
    }

    inline constexpr SDL_Color white = rgb(255, 255, 255);
    inline constexpr SDL_Color black = rgb(0, 0, 0);
    inline constexpr SDL_Color red = rgb(220, 38, 38);
    inline constexpr SDL_Color green = rgb(16, 185, 129);
    inline constexpr SDL_Color blue = rgb(59, 130, 246);
    inline constexpr SDL_Color violet = rgb(255, 0, 255);
    inline constexpr SDL_Color light_grey = rgb(180, 180, 180);
    inline constexpr SDL_Color grey = rgb(122, 122, 122);
    inline constexpr SDL_Color dark_grey = rgb(64, 64, 64);
}

#endif