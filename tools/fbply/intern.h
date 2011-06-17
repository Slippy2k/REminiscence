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

#ifndef __INTERN_H__
#define __INTERN_H__

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>

#include "sys.h"
#include "util.h"

#define ABS(x) (((x)<0)?-(x):(x))
#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

template<typename T>
inline void SWAP(T &a, T &b) {
	T tmp = a; 
	a = b;
	b = tmp;
}

struct Ptr {
	uint8 *pc;
	
	uint8 fetchByte() {
		return *pc++;
	}
	
	uint16 fetchWord() {
		uint16 i = READ_BE_UINT16(pc);
		pc += 2;
		return i;
	}
};

struct Point {
	int16 x;
	int16 y;
};

struct Vector {
	int16 x;
	int16 y;

	void set(const Point &pt1, const Point &pt2) {
		x = pt1.x - pt2.x;
		y = pt1.y - pt2.y;
	}

	int perp_dot(const Vector &v) const {
		//	http://mathworld.wolfram.com/PerpDotProduct.html
		return y * v.x - x * v.y;
	}

	int dot(const Vector &v) const {
		return x * v.x + y * v.y;
	}	
};

struct Color {
	uint8 r;
	uint8 g;
	uint8 b;
};

struct Buf {
	uint8 *buf;
	uint32 size;
};

#endif
