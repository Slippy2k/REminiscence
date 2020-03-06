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

#include <SDL.h>
#include <SDL_opengl.h>
#include <vector>
#include "systemstub.h"


struct DrawListEntry {
	uint8 color;
	uint32 listIndex;
};

struct OGLStub : SystemStub {
	typedef std::vector<DrawListEntry> DrawList;
	
	enum {
		SCREEN_W      = 239,
		SCREEN_H      = 127,
		DEFAULT_SCALE = 3,
		MAX_TESS_VERT = 150
	};

	Color _pal[256];
	SDL_Surface *_screen;
	GLUtesselator *_tobj;
	uint16 _numTessVert;
	GLdouble _tessVert[MAX_TESS_VERT][3];
	DrawList _drawList, _drawListBak;

	virtual void init(const char *title);
	virtual void destroy();
	virtual void sleep(uint32 duration);
	virtual void processEvents();
	virtual uint32 getTimeStamp();
	virtual void setPaletteEntry(uint8 i, const Color *col);
	virtual void addEllipseToList(uint8 color, const Point *pt, uint16 rx, uint16 ry);
	virtual void addPointToList(uint8 color, const Point *pt);
	virtual void addPolygonToList(uint8 color, const Point *pts, uint8 numPts);
	virtual void blitList();
	virtual void copyList();
	virtual void saveList();
	virtual void clearList();

	bool isPolygonConvex(const Point *pt, uint8 numPts);
	void resize(uint16 w, uint16 h);
};

void APIENTRY tessVertexCallback(void *vertex, void *userData) {
	GLdouble *pv = (GLdouble *)vertex;
	debug(DBG_SYSSTUB, "tessVertexCallback(%f, %f, %f)\n", pv[0], pv[1], pv[2]);
	glVertex3d(pv[0], pv[1], pv[2]);
}

void APIENTRY tessBeginCallback(GLenum which, void *userData) {
	debug(DBG_SYSSTUB, "tessBeginCallback(which = %d)", which);
	glBegin(which);
}
	
void APIENTRY tessEndCallback(void *userData) {
	debug(DBG_SYSSTUB, "tessEndCallback()");
	glEnd();
}

void APIENTRY tessCombineCallback(GLdouble coords[3], GLdouble *vertexData[4], GLfloat weight[4], GLdouble **dataOut, void *userData) {
	debug(DBG_SYSSTUB, "tessCombineCallback()");
	OGLStub *stub = (OGLStub *)userData;
	assert(stub->_numTessVert < OGLStub::MAX_TESS_VERT);
	GLdouble *pv = stub->_tessVert[stub->_numTessVert];
	++stub->_numTessVert;
	memcpy(pv, coords, 3 * sizeof(GLdouble));
	*dataOut = pv;
}

void APIENTRY tessErrorCallback(GLenum errno, void *userData) {
	const GLubyte *estr = gluErrorString(errno);
	error("OGLStub::tessErrorCallback() err = %s", estr);
}

SystemStub *SystemStub_OGL_create() {
	return new OGLStub();
}

void OGLStub::init(const char *title) {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_WM_SetCaption(title, NULL);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	memset(&_pi, 0, sizeof(_pi));
	int w = SCREEN_W * DEFAULT_SCALE;
	int h = SCREEN_H * DEFAULT_SCALE;
	_screen = SDL_SetVideoMode(w, h, 16, SDL_OPENGL | SDL_RESIZABLE);
	resize(w, h);
	_tobj = gluNewTess(); 
	gluTessCallback(_tobj, GLU_TESS_VERTEX_DATA,  (_GLUfuncptr)&tessVertexCallback);
	gluTessCallback(_tobj, GLU_TESS_BEGIN_DATA,   (_GLUfuncptr)&tessBeginCallback);
	gluTessCallback(_tobj, GLU_TESS_END_DATA,     (_GLUfuncptr)&tessEndCallback);
	gluTessCallback(_tobj, GLU_TESS_COMBINE_DATA, (_GLUfuncptr)&tessCombineCallback);
	gluTessCallback(_tobj, GLU_TESS_ERROR_DATA,   (_GLUfuncptr)&tessErrorCallback);
	gluTessProperty(_tobj, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_POSITIVE);
}

void OGLStub::destroy() {
	gluDeleteTess(_tobj);
	SDL_FreeSurface(_screen);
	SDL_Quit();
}

void OGLStub::sleep(uint32 duration) {
	Uint32 now = SDL_GetTicks();
	processEvents();
	Uint32 tev = SDL_GetTicks() - now;
	if (tev < duration) {
		SDL_Delay(duration - tev);
	}
}

void OGLStub::processEvents() {
	SDL_Event ev;
	while (SDL_PollEvent(&ev)) {
		switch (ev.type) {
		case SDL_QUIT:
			exit(1);
			break;
		case SDL_VIDEORESIZE:
			_screen = SDL_SetVideoMode(ev.resize.w, ev.resize.h, 16, SDL_OPENGL | SDL_RESIZABLE);
			resize(ev.resize.w, ev.resize.h);
			break;			
		}
	}	
}

uint32 OGLStub::getTimeStamp() {
	return SDL_GetTicks();
}

void OGLStub::setPaletteEntry(uint8 i, const Color *col) {
	_pal[i].r = (col->r << 2) | (col->r & 3);
	_pal[i].g = (col->g << 2) | (col->g & 3);
	_pal[i].b = (col->b << 2) | (col->b & 3);
}

void OGLStub::addEllipseToList(uint8 color, const Point *pt, uint16 rx, uint16 ry) {
	debug(DBG_SYSSTUB, "OGLStub::addEllipseToList(%d, %d, %d, %d)", pt->x, pt->y, rx, ry);	
	DrawListEntry e;
	e.color = color;
	e.listIndex = glGenLists(1);
	glNewList(e.listIndex, GL_COMPILE);
		GLUquadricObj *qobj = gluNewQuadric();
		gluQuadricDrawStyle(qobj, GLU_FILL);
		glPushMatrix();
		glLoadIdentity();
		glTranslatef(pt->x, SCREEN_H - pt->y, 0.);
		float fx, fy, r;
		if (rx > ry) {
			r = rx;
			fx = 1.;
			fy = (float)ry / rx;
		} else {
			r = ry;
			fx = (float)rx / ry;
			fy = 1.;
		}
		glScalef(fx, fy, 1.f);
		gluDisk(qobj, 0., r, 100, 1);
		glPopMatrix();
		gluDeleteQuadric(qobj);
	glEndList();
	_drawList.push_back(e);
}

void OGLStub::addPointToList(uint8 color, const Point *pt) {
	debug(DBG_SYSSTUB, "OGLStub::addPointToList(%d, %d)", pt->x, pt->y);
	DrawListEntry e;
	e.color = color;
	e.listIndex = glGenLists(1);
	glNewList(e.listIndex, GL_COMPILE);
		glBegin(GL_POINTS);
			glVertex2i(pt->x, SCREEN_H - pt->y);
		glEnd();
	glEndList();
	_drawList.push_back(e);
}

void OGLStub::addPolygonToList(uint8 color, const Point *pts, uint8 numPts) {
	debug(DBG_SYSSTUB, "OGLStub::addPolygonToList(%d)", numPts);
	DrawListEntry e;
	e.color = color;
	e.listIndex = glGenLists(1);
	glNewList(e.listIndex, GL_COMPILE);
	if (numPts == 2) {
		glBegin(GL_LINES);
			glVertex2i(pts[0].x, SCREEN_H - pts[0].y);
			glVertex2i(pts[1].x, SCREEN_H - pts[1].y);
		glEnd();
	} else if (numPts == 3) {
		glBegin(GL_TRIANGLES);
			glVertex2i(pts[0].x, SCREEN_H - pts[0].y);
			glVertex2i(pts[1].x, SCREEN_H - pts[1].y);
			glVertex2i(pts[2].x, SCREEN_H - pts[2].y);
		glEnd();
	} else {
		debug(DBG_SYSSTUB, "numPts = %d e = 0x%X listIndex = 0x%X", numPts, &e, e.listIndex);
#ifdef DEBUG_POLYGON
		glBegin(GL_LINE_LOOP);
			while (numPts--) {
				glVertex2i(pts->x, SCREEN_H - pts->y);
				++pts;
			}
		glEnd();
#else
		memset(_tessVert, 0, sizeof(_tessVert));
		gluTessBeginPolygon(_tobj, this);
			gluTessBeginContour(_tobj);
				for (_numTessVert = 0; _numTessVert < numPts; ++_numTessVert, ++pts) {
					assert(_numTessVert < MAX_TESS_VERT);
					GLdouble *v = _tessVert[_numTessVert];
					v[0] = pts->x;
					v[1] = SCREEN_H - pts->y;
					v[2] = 0;
					gluTessVertex(_tobj, v, v);
				}
			gluTessEndContour(_tobj);
		gluTessEndPolygon(_tobj);
#endif
	}
	glEndList();	
	_drawList.push_back(e);
}

void OGLStub::blitList() {
	glClearColor(0, 0, 0, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glShadeModel(GL_SMOOTH);
	DrawList::const_iterator it = _drawList.begin();
	for (; it != _drawList.end(); ++it) {
		DrawListEntry entry = *it;
		Color *col = &_pal[entry.color];
		glColor3f(col->r / 255.f, col->g / 255.f, col->b / 255.f);
		glCallList(entry.listIndex);
	}
	SDL_GL_SwapBuffers();
}

void OGLStub::copyList() {
	debug(DBG_SYSSTUB, "OGLStub::copyList()");
	_drawList = _drawListBak;
}

void OGLStub::saveList() {
	debug(DBG_SYSSTUB, "OGLStub::saveList() size = %d", _drawList.size());
	_drawListBak = _drawList;
}

void OGLStub::clearList() {
	debug(DBG_SYSSTUB, "OGLStub::clearList() size = %d", _drawList.size());
	_drawList.clear();
}

void OGLStub::resize(uint16 w, uint16 h) {	
	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
   	glOrtho(0, SCREEN_W, 0, SCREEN_H, 0, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	float r = (float)w / SCREEN_W;
	glLineWidth(r);
	glPointSize(r);
}

bool OGLStub::isPolygonConvex(const Point *pt, uint8 numPts) {
	// http://mathworld.wolfram.com/ConvexPolygon.html
	bool convex = false;
	if (numPts > 2) {
		Vector v1, v2;
		v1.set(pt[0], pt[1]);
		v2.set(pt[1], pt[2]);
		int sign = v1.perp_dot(v2);
		convex = true;
		for (int i = 2; i < numPts && convex; ++i) {
			v1 = v2;
			if (i == numPts - 1) {
				v2.set(pt[i], pt[0]);
			} else {
				v2.set(pt[i], pt[i + 1]);
			}
			int s = v1.perp_dot(v2);
			if (s * sign < 0) { // different sign
				convex = false;
			}
		}
		if (convex) {
			v1 = v2;
			v2.set(pt[0], pt[1]);
			int s = v1.perp_dot(v2);
			if (s * sign < 0) { // different sign
				convex = false;
			}
		}
	}
	return convex;
}
