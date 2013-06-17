/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */


#include "engines/wintermute/math/offset_tools.h"
#include <math.h>

namespace Wintermute {
	Common::Point OffsetTools::rotate_point(Common::Point point, float rotate) {
		/* 
		 * Returns the coordinates for a point after rotation 
		 */
		float rotate_rad = rotate * M_PI / 180;
		Common::Point newpoint;
		int flipY = -point.y;
		newpoint.x = point.x * cos(rotate_rad) - flipY * sin(rotate_rad);
		newpoint.y = point.x * sin(rotate_rad) + flipY * cos(rotate_rad);
		newpoint.y = -newpoint.y;
		/* I apply the textbook formula, but first I reverse the Y-axis, otherwise
		   I'd be performing a rotation in the wrong direction */
		return newpoint;
	};

	Common::Point OffsetTools::compute_box_offset(Rect32 rect, float rotate, float zoomX, float zoomY) {
		/* 
		 * Computes the coordinates for the origin of a sprite (=NW corner) after 
		 * rotation.
		 * The old origin ends up further down and/or right in the resulting sprite
		 * because of the "padding" introducted by the rotation, since we can't have
		 * points with negative coordinates.
		 */
		Common::Point ne (rect.right, 0);
		Common::Point se (rect.right, rect.bottom);
		Common::Point sw (0, rect.bottom);
		Common::Point offset;

		/*
		 * In each quadrant of the unit circle a different corner is "pushing"
		 * the boundaries of the sprite; I work on a per-case basis.
		 */
		if (0 <= rotate && rotate < 90) {
			offset.x = 0;
			offset.y = rotate_point (ne, rotate).y;
		}

		if (90 <= rotate && rotate < 180) {
			offset.x = rotate_point(ne, rotate).x;
			offset.y = rotate_point(se, rotate).y;
		}
		if (180 <= rotate && rotate < 270) {
			offset.x = rotate_point(se, rotate).x;
			offset.y = rotate_point(sw, rotate).y;
		}
		if (270 <= rotate && rotate < 360){
			offset.x = rotate_point(sw, rotate).x;
			offset.y = 0;
		}

		// I then "stretch" for zoom

		offset.x = offset.x * zoomX / 100;
		offset.y = offset.y * zoomY / 100;
		
		return offset;
	}
}
