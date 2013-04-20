/*

SDL_rotozoom.c: rotozoomer, zoomer and shrinker for 32bit or 8bit surfaces

Copyright (C) 2001-2012  Andreas Schiffler

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.

Andreas Schiffler -- aschiffler at ferzkopp dot net

*/


// THIS FILE HAS BEEN STRIPPED DOWN FOR USAGE IN SCUMMVM, AND IS THUS AN ALTERED VERSION

#ifndef _SDL_rotozoom_h
#define _SDL_rotozoom_h

#include "graphics/surface.h"

namespace RotoZoom {

/* ---- Defines */

/*!
\brief Disable anti-aliasing (no smoothing).
*/
#define SMOOTHING_OFF       0

/*!
\brief Enable anti-aliasing (smoothing).
*/
#define SMOOTHING_ON        1

extern void rotozoomSurfaceSize(int width, int height, double angle, double zoom, int *dstwidth, int *dstheight);
// RotoZoom-functions:
extern Graphics::Surface *rotozoomSurface(Graphics::Surface *src, double angle, double zoom, int smooth);
extern Graphics::Surface *rotozoomSurfaceXY(Graphics::Surface *src, double angle, double zoomx, double zoomy, int smooth);
extern void rotozoomSurfaceSize(int width, int height, double angle, double zoom, int *dstwidth, int *dstheight);
extern void rotozoomSurfaceSizeXY(int width, int height, double angle, double zoomx, double zoomy, int *dstwidth, int *dstheight);

} // end of namespace RotoZoom

#endif              /* _SDL_rotozoom_h */
