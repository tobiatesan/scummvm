// All the pure zoom and 8-bit functionality is removed from this version.

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

#include "common/textconsole.h"
#include "common/util.h"
#include "SDL_rotozoom.h"


namespace RotoZoom {
/* ---- Internally used structures */

/*!
\brief A 32 bit RGBA pixel.
*/
struct tColorRGBA {
	uint8 r;
	uint8 g;
	uint8 b;
	uint8 a;
};

/*!
\brief Number of guard rows added to destination surfaces.

This is a simple but effective workaround for observed issues.
These rows allocate extra memory and are then hidden from the surface.
Rows are added to the end of destination surfaces when they are allocated.
This catches any potential overflows which seem to happen with
just the right src image dimensions and scale/rotation and can lead
to a situation where the program can segfault.
*/
#define GUARD_ROWS (2)

/*!
\brief Lower limit of absolute zoom factor or rotation degrees.
*/
#define VALUE_LIMIT 0.001

/*!
\brief Internal 32 bit Zoomer with optional anti-aliasing by bilinear interpolation.

Zooms 32 bit RGBA/ABGR 'src' surface to 'dst' surface.
Assumes src and dst surfaces are of 32 bit depth.
Assumes dst surface was allocated with the correct dimensions.

\param src The surface to zoom (input).
\param dst The zoomed surface (output).
\param flipx Flag indicating if the image should be horizontally flipped.
\param flipy Flag indicating if the image should be vertically flipped.
\param smooth Antialiasing flag; set to SMOOTHING_ON to enable.

\return 0 for success or -1 for error.
*/
int _zoomSurfaceRGBA(Graphics::Surface *src, Graphics::Surface *dst, int flipx, int flipy, int smooth) {
	int x, y, sx, sy, ssx, ssy, *sax, *say, *csax, *csay, *salast, csx, csy, ex, ey, cx, cy, sstep, sstepx, sstepy;
	tColorRGBA *c00, *c01, *c10, *c11;
	tColorRGBA *sp, *csp, *dp;
	int spixelgap, spixelw, spixelh, dgap, t1, t2;

	/*
	* Allocate memory for row/column increments
	*/
	if ((sax = (int *) malloc((dst->w + 1) * sizeof(uint32))) == NULL) {
		return (-1);
	}
	if ((say = (int *) malloc((dst->h + 1) * sizeof(uint32))) == NULL) {
		free(sax);
		return (-1);
	}

	/*
	* Precalculate row increments
	*/
	spixelw = (src->w - 1);
	spixelh = (src->h - 1);
	if (smooth) {
		sx = (int)(65536.0 * (float) spixelw / (float)(dst->w - 1));
		sy = (int)(65536.0 * (float) spixelh / (float)(dst->h - 1));
	} else {
		sx = (int)(65536.0 * (float)(src->w) / (float)(dst->w));
		sy = (int)(65536.0 * (float)(src->h) / (float)(dst->h));
	}

	/* Maximum scaled source size */
	ssx = (src->w << 16) - 1;
	ssy = (src->h << 16) - 1;

	/* Precalculate horizontal row increments */
	csx = 0;
	csax = sax;
	for (x = 0; x <= dst->w; x++) {
		*csax = csx;
		csax++;
		csx += sx;

		/* Guard from overflows */
		if (csx > ssx) {
			csx = ssx;
		}
	}

	/* Precalculate vertical row increments */
	csy = 0;
	csay = say;
	for (y = 0; y <= dst->h; y++) {
		*csay = csy;
		csay++;
		csy += sy;

		/* Guard from overflows */
		if (csy > ssy) {
			csy = ssy;
		}
	}

	sp = (tColorRGBA *) src->pixels;
	dp = (tColorRGBA *) dst->pixels;
	dgap = dst->pitch - dst->w * 4;
	spixelgap = src->pitch / 4;

	if (flipx) {
		sp += spixelw;
	}
	if (flipy) {
		sp += (spixelgap * spixelh);
	}

	/*
	* Switch between interpolating and non-interpolating code
	*/
	if (smooth) {

		/*
		* Interpolating Zoom
		*/
		csay = say;
		for (y = 0; y < dst->h; y++) {
			csp = sp;
			csax = sax;
			for (x = 0; x < dst->w; x++) {
				/*
				* Setup color source pointers
				*/
				ex = (*csax & 0xffff);
				ey = (*csay & 0xffff);
				cx = (*csax >> 16);
				cy = (*csay >> 16);
				sstepx = cx < spixelw;
				sstepy = cy < spixelh;
				c00 = sp;
				c01 = sp;
				c10 = sp;
				if (sstepy) {
					if (flipy) {
						c10 -= spixelgap;
					} else {
						c10 += spixelgap;
					}
				}
				c11 = c10;
				if (sstepx) {
					if (flipx) {
						c01--;
						c11--;
					} else {
						c01++;
						c11++;
					}
				}

				/*
				* Draw and interpolate colors
				*/
				t1 = ((((c01->r - c00->r) * ex) >> 16) + c00->r) & 0xff;
				t2 = ((((c11->r - c10->r) * ex) >> 16) + c10->r) & 0xff;
				dp->r = (((t2 - t1) * ey) >> 16) + t1;
				t1 = ((((c01->g - c00->g) * ex) >> 16) + c00->g) & 0xff;
				t2 = ((((c11->g - c10->g) * ex) >> 16) + c10->g) & 0xff;
				dp->g = (((t2 - t1) * ey) >> 16) + t1;
				t1 = ((((c01->b - c00->b) * ex) >> 16) + c00->b) & 0xff;
				t2 = ((((c11->b - c10->b) * ex) >> 16) + c10->b) & 0xff;
				dp->b = (((t2 - t1) * ey) >> 16) + t1;
				t1 = ((((c01->a - c00->a) * ex) >> 16) + c00->a) & 0xff;
				t2 = ((((c11->a - c10->a) * ex) >> 16) + c10->a) & 0xff;
				dp->a = (((t2 - t1) * ey) >> 16) + t1;
				/*
				* Advance source pointer x
				*/
				salast = csax;
				csax++;
				sstep = (*csax >> 16) - (*salast >> 16);
				if (flipx) {
					sp -= sstep;
				} else {
					sp += sstep;
				}

				/*
				* Advance destination pointer x
				*/
				dp++;
			}
			/*
			* Advance source pointer y
			*/
			salast = csay;
			csay++;
			sstep = (*csay >> 16) - (*salast >> 16);
			sstep *= spixelgap;
			if (flipy) {
				sp = csp - sstep;
			} else {
				sp = csp + sstep;
			}

			/*
			* Advance destination pointer y
			*/
			dp = (tColorRGBA *)((uint8 *) dp + dgap);
		}
	} else {
		/*
		* Non-Interpolating Zoom
		*/
		csay = say;
		for (y = 0; y < dst->h; y++) {
			csp = sp;
			csax = sax;
			for (x = 0; x < dst->w; x++) {
				/*
				* Draw
				*/
				*dp = *sp;

				/*
				* Advance source pointer x
				*/
				salast = csax;
				csax++;
				sstep = (*csax >> 16) - (*salast >> 16);
				if (flipx) {
					sstep = -sstep;
				}
				sp += sstep;

				/*
				* Advance destination pointer x
				*/
				dp++;
			}
			/*
			* Advance source pointer y
			*/
			salast = csay;
			csay++;
			sstep = (*csay >> 16) - (*salast >> 16);
			sstep *= spixelgap;
			if (flipy) {
				sstep = -sstep;
			}
			sp = csp + sstep;

			/*
			* Advance destination pointer y
			*/
			dp = (tColorRGBA *)((uint8 *) dp + dgap);
		}
	}

	/*
	* Remove temp arrays
	*/
	free(sax);
	free(say);

	return (0);
}

/*!
\brief Internal 32 bit rotozoomer with optional anti-aliasing.

Rotates and zooms 32 bit RGBA/ABGR 'src' surface to 'dst' surface based on the control
parameters by scanning the destination surface and applying optionally anti-aliasing
by bilinear interpolation.
Assumes src and dst surfaces are of 32 bit depth.
Assumes dst surface was allocated with the correct dimensions.

\param src Source surface.
\param dst Destination surface.
\param cx Horizontal center coordinate.
\param cy Vertical center coordinate.
\param isin Integer version of sine of angle.
\param icos Integer version of cosine of angle.
\param flipx Flag indicating horizontal mirroring should be applied.
\param flipy Flag indicating vertical mirroring should be applied.
\param smooth Flag indicating anti-aliasing should be used.
*/
void _transformSurfaceRGBA(Graphics::Surface *src, Graphics::Surface *dst, int cx, int cy, int isin, int icos, int flipx, int flipy, int smooth) {
	int x, y, t1, t2, dx, dy, xd, yd, sdx, sdy, ax, ay, ex, ey, sw, sh;
	tColorRGBA c00, c01, c10, c11, cswap;
	tColorRGBA *pc, *sp;
	int gap;

	/*
	* Variable setup
	*/
	xd = ((src->w - dst->w) << 15);
	yd = ((src->h - dst->h) << 15);
	ax = (cx << 16) - (icos * cx);
	ay = (cy << 16) - (isin * cx);
	sw = src->w - 1;
	sh = src->h - 1;
	pc = (tColorRGBA *) dst->pixels;
	gap = dst->pitch - dst->w * 4;

	/*
	* Switch between interpolating and non-interpolating code
	*/
	if (smooth) {
		for (y = 0; y < dst->h; y++) {
			dy = cy - y;
			sdx = (ax + (isin * dy)) + xd;
			sdy = (ay - (icos * dy)) + yd;
			for (x = 0; x < dst->w; x++) {
				dx = (sdx >> 16);
				dy = (sdy >> 16);
				if (flipx) {
					dx = sw - dx;
				}
				if (flipy) {
					dy = sh - dy;
				}
				if ((dx > -1) && (dy > -1) && (dx < (src->w - 1)) && (dy < (src->h - 1))) {
					sp = (tColorRGBA *)src->pixels;;
					sp += ((src->pitch / 4) * dy);
					sp += dx;
					c00 = *sp;
					sp += 1;
					c01 = *sp;
					sp += (src->pitch / 4);
					c11 = *sp;
					sp -= 1;
					c10 = *sp;
					if (flipx) {
						cswap = c00;
						c00 = c01;
						c01 = cswap;
						cswap = c10;
						c10 = c11;
						c11 = cswap;
					}
					if (flipy) {
						cswap = c00;
						c00 = c10;
						c10 = cswap;
						cswap = c01;
						c01 = c11;
						c11 = cswap;
					}
					/*
					* Interpolate colors
					*/
					ex = (sdx & 0xffff);
					ey = (sdy & 0xffff);
					t1 = ((((c01.r - c00.r) * ex) >> 16) + c00.r) & 0xff;
					t2 = ((((c11.r - c10.r) * ex) >> 16) + c10.r) & 0xff;
					pc->r = (((t2 - t1) * ey) >> 16) + t1;
					t1 = ((((c01.g - c00.g) * ex) >> 16) + c00.g) & 0xff;
					t2 = ((((c11.g - c10.g) * ex) >> 16) + c10.g) & 0xff;
					pc->g = (((t2 - t1) * ey) >> 16) + t1;
					t1 = ((((c01.b - c00.b) * ex) >> 16) + c00.b) & 0xff;
					t2 = ((((c11.b - c10.b) * ex) >> 16) + c10.b) & 0xff;
					pc->b = (((t2 - t1) * ey) >> 16) + t1;
					t1 = ((((c01.a - c00.a) * ex) >> 16) + c00.a) & 0xff;
					t2 = ((((c11.a - c10.a) * ex) >> 16) + c10.a) & 0xff;
					pc->a = (((t2 - t1) * ey) >> 16) + t1;
				}
				sdx += icos;
				sdy += isin;
				pc++;
			}
			pc = (tColorRGBA *)((uint8 *) pc + gap);
		}
	} else {
		for (y = 0; y < dst->h; y++) {
			dy = cy - y;
			sdx = (ax + (isin * dy)) + xd;
			sdy = (ay - (icos * dy)) + yd;
			for (x = 0; x < dst->w; x++) {
				dx = (short)(sdx >> 16);
				dy = (short)(sdy >> 16);
				if (flipx) {
					dx = (src->w - 1) - dx;
				}
				if (flipy) {
					dy = (src->h - 1) - dy;
				}
				if ((dx >= 0) && (dy >= 0) && (dx < src->w) && (dy < src->h)) {
					sp = (tColorRGBA *)((uint8 *) src->pixels + src->pitch * dy);
					sp += dx;
					*pc = *sp;
				}
				sdx += icos;
				sdy += isin;
				pc++;
			}
			pc = (tColorRGBA *)((uint8 *) pc + gap);
		}
	}
}

/*!
\brief Internal target surface sizing function for rotozooms with trig result return.

\param width The source surface width.
\param height The source surface height.
\param angle The angle to rotate in degrees.
\param zoomx The horizontal scaling factor.
\param zoomy The vertical scaling factor.
\param dstwidth The calculated width of the destination surface.
\param dstheight The calculated height of the destination surface.
\param canglezoom The sine of the angle adjusted by the zoom factor.
\param sanglezoom The cosine of the angle adjusted by the zoom factor.

*/
void _rotozoomSurfaceSizeTrig(int width, int height, double angle, double zoomx, double zoomy, int *dstwidth, int *dstheight, double *canglezoom, double *sanglezoom) {
	double x, y, cx, cy, sx, sy;
	double radangle;
	int dstwidthhalf, dstheighthalf;

	/*
	* Determine destination width and height by rotating a centered source box
	*/
	radangle = angle * (M_PI / 180.0);
	*sanglezoom = sin(radangle);
	*canglezoom = cos(radangle);
	*sanglezoom *= zoomx;
	*canglezoom *= zoomx;
	x = (double)(width / 2);
	y = (double)(height / 2);
	cx = *canglezoom * x;
	cy = *canglezoom * y;
	sx = *sanglezoom * x;
	sy = *sanglezoom * y;

	dstwidthhalf = MAX((int)
					   ceil(MAX(MAX(MAX(fabs(cx + sy), fabs(cx - sy)), fabs(-cx + sy)), fabs(-cx - sy))), 1);
	dstheighthalf = MAX((int)
						ceil(MAX(MAX(MAX(fabs(sx + cy), fabs(sx - cy)), fabs(-sx + cy)), fabs(-sx - cy))), 1);
	*dstwidth = 2 * dstwidthhalf;
	*dstheight = 2 * dstheighthalf;
}

/*!
\brief Returns the size of the resulting target surface for a rotozoomSurfaceXY() call.

\param width The source surface width.
\param height The source surface height.
\param angle The angle to rotate in degrees.
\param zoomx The horizontal scaling factor.
\param zoomy The vertical scaling factor.
\param dstwidth The calculated width of the rotozoomed destination surface.
\param dstheight The calculated height of the rotozoomed destination surface.
*/
void rotozoomSurfaceSizeXY(int width, int height, double angle, double zoomx, double zoomy, int *dstwidth, int *dstheight) {
	double dummy_sanglezoom, dummy_canglezoom;

	_rotozoomSurfaceSizeTrig(width, height, angle, zoomx, zoomy, dstwidth, dstheight, &dummy_sanglezoom, &dummy_canglezoom);
}

/*!
\brief Returns the size of the resulting target surface for a rotozoomSurface() call.

\param width The source surface width.
\param height The source surface height.
\param angle The angle to rotate in degrees.
\param zoom The scaling factor.
\param dstwidth The calculated width of the rotozoomed destination surface.
\param dstheight The calculated height of the rotozoomed destination surface.
*/
void rotozoomSurfaceSize(int width, int height, double angle, double zoom, int *dstwidth, int *dstheight) {
	double dummy_sanglezoom, dummy_canglezoom;

	_rotozoomSurfaceSizeTrig(width, height, angle, zoom, zoom, dstwidth, dstheight, &dummy_sanglezoom, &dummy_canglezoom);
}

/*!
\brief Rotates and zooms a surface and optional anti-aliasing.

Rotates and zoomes a 32bit or 8bit 'src' surface to newly created 'dst' surface.
'angle' is the rotation in degrees and 'zoom' a scaling factor. If 'smooth' is set
then the destination 32bit surface is anti-aliased. If the surface is not 8bit
or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.

\param src The surface to rotozoom.
\param angle The angle to rotate in degrees.
\param zoom The scaling factor.
\param smooth Antialiasing flag; set to SMOOTHING_ON to enable.

\return The new rotozoomed surface.
*/
Graphics::Surface *rotozoomSurface(Graphics::Surface *src, double angle, double zoom, int smooth) {
	return rotozoomSurfaceXY(src, angle, zoom, zoom, smooth);
}

/*!
\brief Rotates and zooms a surface with different horizontal and vertival scaling factors and optional anti-aliasing.

Rotates and zooms a 32bit or 8bit 'src' surface to newly created 'dst' surface.
'angle' is the rotation in degrees, 'zoomx and 'zoomy' scaling factors. If 'smooth' is set
then the destination 32bit surface is anti-aliased. If the surface is not 8bit
or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.

\param src The surface to rotozoom.
\param angle The angle to rotate in degrees.
\param zoomx The horizontal scaling factor.
\param zoomy The vertical scaling factor.
\param smooth Antialiasing flag; set to SMOOTHING_ON to enable.

\return The new rotozoomed surface.
*/
Graphics::Surface *rotozoomSurfaceXY(Graphics::Surface *src, double angle, double zoomx, double zoomy, int smooth) {
	Graphics::Surface *rz_src;
	Graphics::Surface *rz_dst;
	double zoominv;
	double sanglezoom, canglezoom, sanglezoominv, canglezoominv;
	int dstwidthhalf, dstwidth, dstheighthalf, dstheight;
	bool is32bit;
	int flipx, flipy;

	/*
	* Sanity check
	*/
	if (src == NULL) {
		return (NULL);
	}

	/*
	* Determine if source surface is 32bit or 8bit
	*/
	is32bit = (src->format.bytesPerPixel == 4);
	if ((is32bit) || (src->format.bytesPerPixel == 1)) {
		/*
		* Use source surface 'as is'
		*/
		rz_src = src;
	} else {
		// TODO: Convert the surface?
		error("Rotation only supported for 32-bpp surfaces");
	}

	/*
	* Sanity check zoom factor
	*/
	flipx = (zoomx < 0.0);
	if (flipx) {
		zoomx = -zoomx;
	}
	flipy = (zoomy < 0.0);
	if (flipy) {
		zoomy = -zoomy;
	}
	if (zoomx < VALUE_LIMIT) {
		zoomx = VALUE_LIMIT;
	}
	if (zoomy < VALUE_LIMIT) {
		zoomy = VALUE_LIMIT;
	}
	zoominv = 65536.0 / (zoomx * zoomx);

	/*
	* Check if we have a rotozoom or just a zoom
	*/
	if (fabs(angle) > VALUE_LIMIT) {

		/*
		* Angle!=0: full rotozoom
		*/
		/*
		* -----------------------
		*/

		/* Determine target size */
		_rotozoomSurfaceSizeTrig(rz_src->w, rz_src->h, angle, zoomx, zoomy, &dstwidth, &dstheight, &canglezoom, &sanglezoom);

		/*
		* Calculate target factors from sin/cos and zoom
		*/
		sanglezoominv = sanglezoom;
		canglezoominv = canglezoom;
		sanglezoominv *= zoominv;
		canglezoominv *= zoominv;

		/* Calculate half size */
		dstwidthhalf = dstwidth / 2;
		dstheighthalf = dstheight / 2;

		/*
		* Alloc space to completely contain the rotated surface
		*/
		rz_dst = NULL;
		if (is32bit) {
			/*
			* Target surface is 32bit with source RGBA/ABGR ordering
			*/
			rz_dst = new Graphics::Surface();
			rz_dst->create(dstwidth, dstheight + GUARD_ROWS, rz_src->format);
		} else {
			error("Rotation only supported for 32-bpp surfaces");
		}

		/* Check target */
		if (rz_dst == NULL) {
			return NULL;
		}

		/* Adjust for guard rows */
		rz_dst->h = dstheight;

		/*      if (colorKeyAvailable == 1){
		            colorkey = SDL_MapRGB(rz_dst->format, r, g, b);

		            SDL_FillRect(rz_dst, NULL, colorkey );
		        }*/

		/*
		* Check which kind of surface we have
		*/
		if (is32bit) {
			/*
			* Call the 32bit transformation routine to do the rotation (using alpha)
			*/
			_transformSurfaceRGBA(rz_src, rz_dst, dstwidthhalf, dstheighthalf,
								  (int)(sanglezoominv), (int)(canglezoominv),
								  flipx, flipy,
								  smooth);
			/*
			* Turn on source-alpha support
			*/
//			SDL_SetAlpha(rz_dst, SDL_SRCALPHA, 255);
//			SDL_SetColorKey(rz_dst, SDL_SRCCOLORKEY | SDL_RLEACCEL, _colorkey(rz_src));
		} else {
			error("Rotation only supported for 32-bpp surfaces");
		}

	} else {
		// Angle = 0, just a zoom, handle this elsewhere.
		error("We shouldn't enter here, as pure zooming is handled elsewhere");
	}

	/*
	* Return destination surface
	*/
	return (rz_dst);
}

} // end of namespace RotoZoom
