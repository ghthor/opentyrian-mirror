/* 
 * OpenTyrian Classic: A modern cross-platform port of Tyrian
 * Copyright (C) 2007-2009  The OpenTyrian Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef FONT_H
#define FONT_H

#include "SDL.h"

typedef enum
{
	large_font = 0,
	normal_font = 1,
	small_font = 2,
}
Font;

typedef enum
{
	left_aligned,
	centered,
	right_aligned,
}
FontAlignment;

void font_draw_hv( SDL_Surface *surface, int x, int y, const char *text, Font font, FontAlignment alignment, Uint8 hue, Sint8 value );

#endif // FONT_H
