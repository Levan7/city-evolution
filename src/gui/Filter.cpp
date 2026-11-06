/*
Copyright (C) 2005 Matthias Braun <matze@braunis.de>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/**
 * @author Matthias Braun
 * @file Filter.hpp
 */

#include "Filter.hpp"

#include <stdint.h>
#include <stdexcept>

void color2Grey(SDL_Surface* surface)
{
    const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(surface->format);

    if(format->bits_per_pixel != 32)
        throw std::runtime_error("Can only convert 32 bit images to greyscale");
    
    SDL_LockSurface(surface);
    
    uint8_t* p = (uint8_t*) surface->pixels;
    for(int y = 0; y < surface->h; ++y) {
        uint32_t* pixel = (uint32_t*) p;
        for(int x = 0; x < surface->w; ++x) {
            float red 
                = (*pixel & format->Rmask) >> format->Rshift;
            float green
                = (*pixel & format->Gmask) >> format->Gshift;
            float blue
                = (*pixel & format->Bmask) >> format->Bshift;
            
            float greyvalf = 0.3 * red + 0.59 * green + 0.11 * blue;
            uint32_t greyval = (uint32_t) greyvalf;
            *pixel = (*pixel & format->Amask)
                | (greyval << format->Rshift)
                | (greyval << format->Gshift)
                | (greyval << format->Bshift);
            pixel++;
        }
        p += surface->pitch;
    }
    SDL_UnlockSurface(surface);
}


/** @file gui/Filter.cpp */

