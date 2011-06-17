/* fbply - Flashback Cutscene Player
 * Copyright (C) 2004 Gregory Montoir
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __SYSTEMSTUB_H__
#define __SYSTEMSTUB_H__

#include "intern.h"

struct PlayerInput {
	enum {
		DIR_LEFT  = 1 << 0,
		DIR_RIGHT = 1 << 1,
		DIR_UP    = 1 << 2,
		DIR_DOWN  = 1 << 3
	};

	uint8 dirMask;
	bool button;
	bool quit;
};

struct SystemStub {
	PlayerInput _pi;
	
	virtual ~SystemStub() {}
	virtual void init(const char *title) = 0;
	virtual void destroy() = 0;
	virtual void sleep(uint32 duration) = 0;
	virtual void processEvents() = 0;
	virtual uint32 getTimeStamp() = 0;
	virtual void setPaletteEntry(uint8 i, const Color *col) = 0;
	virtual void addEllipseToList(uint8 color, const Point *pt, uint16 rx, uint16 ry) = 0;
	virtual void addPointToList(uint8 color, const Point *pt) = 0;
	virtual void addPolygonToList(uint8 color, const Point *pts, uint8 numPts) = 0;
	virtual void blitList() = 0;
	virtual void copyList() = 0;
	virtual void saveList() = 0;
	virtual void clearList() = 0;
};

extern SystemStub *SystemStub_SDL_create();
extern SystemStub *SystemStub_OGL_create();
extern SystemStub *SystemStub_W32_create();

#endif
