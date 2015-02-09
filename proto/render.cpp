
#ifdef USE_GLES
#include <GLES/gl.h>
#else
#include <SDL.h>
#include <SDL_opengl.h>
#endif
#ifdef _WIN32
#include <windows.h>
#define DYNLIB_SYMBOL DECLSPEC_EXPORT
#else
#define DYNLIB_SYMBOL
#endif
#include <math.h>
#include <sys/time.h>
#include "render.h"
#include "resource_data.h"
#include "scaler.h"

static const int _scalerFactor = 2;
static void (*_scalerProc)(uint16_t *, int, const uint16_t *, int, int, int) = scale2x;

static int roundPowerOfTwo(int x) {
	if ((x & (x - 1)) == 0) {
		return x;
	} else {
		int num = 1;
		while (num < x) {
			num <<= 1;
		}
		return num;
	}
}

TextureCache::TextureCache() {
	memset(&_title, 0, sizeof(_title));
	_title.texId = -1;
	memset(&_background, 0, sizeof(_background));
	_background.texId = -1;
	memset(&_font, 0, sizeof(_font));
	_font.texId = -1;
	memset(&_texturesList, 0, sizeof(_texturesList));
	for (int i = 0; i < ARRAYSIZE(_texturesList); ++i) {
		_texturesList[i].texId = -1;
	}
	memset(&_gfxImagesQueue, 0, sizeof(_gfxImagesQueue));
	_gfxImagesCount = 0;
	if (_scalerFactor != 1) {
		const int w = roundPowerOfTwo(kW * _scalerFactor);
		const int h = roundPowerOfTwo(kH * _scalerFactor);
		_texScaleBuf = (uint16_t *)malloc(w * h * sizeof(uint16_t));
	}
}

void TextureCache::updatePalette(Color *clut) {
	for (int i = 0; i < 256; ++i) {
		_tex8to565[i] = ((clut[i].r >> 3) << 11) | ((clut[i].g >> 2) << 5) | (clut[i].b >> 3);
		_tex8to5551[i] = ((clut[i].r >> 3) << 11) | ((clut[i].g >> 3) << 6) | ((clut[i].b >> 3) << 1);
	}
}

uint16_t *TextureCache::rescaleTexture(DecodeBuffer *buf, int w, int h) {
	if (_scalerFactor == 1) {
		return (uint16_t *)buf->ptr;
	} else {
		_scalerProc(_texScaleBuf, w, (const uint16_t *)buf->ptr, buf->pitch, buf->w, buf->h);
		return _texScaleBuf;
	}
}

static void convertTextureFont(DecodeBuffer *buf, int x, int y, int w, int h, uint8_t color) {
	const int offset = (y * buf->pitch + buf->x + x) * sizeof(uint16_t);
	switch (color) {
	case 0xC0:
		*(uint16_t *)(buf->ptr + offset) = 1;
		break;
	case 0xC1:
		*(uint16_t *)(buf->ptr + offset) = 0xFFFF;
		break;
	}
}

void TextureCache::createTextureFont(ResourceData &res) {
	_font.charsCount = 105;
	const int w = _font.charsCount * 16 * _scalerFactor;
	const int h = 16 * _scalerFactor;
	const int texW = roundPowerOfTwo(w);
	const int texH = roundPowerOfTwo(h);
	if (_font.texId == -1) {
		glGenTextures(1, &_font.texId);
		glBindTexture(GL_TEXTURE_2D, _font.texId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, 0);
		_font.u = w / (GLfloat)texW;
		_font.v = h / (GLfloat)texH;
	}
	DecodeBuffer buf;
	memset(&buf, 0, sizeof(buf));
	if (_scalerFactor == 1) {
		buf.w = buf.pitch = texW;
		buf.h = texH;
	} else {
		buf.w = buf.pitch = _font.charsCount * 16;
		buf.h = 16;
	}
	buf.setPixel = convertTextureFont;
	buf.ptr = (uint8_t *)calloc(buf.w * buf.h, sizeof(uint16_t));
	if (buf.ptr) {
		for (int i = 0; i < _font.charsCount; ++i) {
			res.decodeImageData(res._fnt, i, &buf);
			buf.x += 16;
		}
		const uint16_t *texData = rescaleTexture(&buf, texW, texH);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW, texH, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, texData);
		free(buf.ptr);
	}
}

static void convertTextureTitle(DecodeBuffer *buf, int x, int y, int w, int h, uint8_t color) {
	const uint16_t *lut = (const uint16_t *)buf->lut;
	const int offset = (y * buf->pitch + x) * sizeof(uint16_t);
	*(uint16_t *)(buf->ptr + offset) = lut[color];
}

void TextureCache::createTextureTitle(ResourceData &res, int num) {
	const int w = kW * _scalerFactor;
	const int h = kH * _scalerFactor;
	const int texW = roundPowerOfTwo(w);
	const int texH = roundPowerOfTwo(h);
	if (_title.texId == -1) {
		glGenTextures(1, &_title.texId);
		glBindTexture(GL_TEXTURE_2D, _title.texId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texW, texH, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 0);
		_title.u = w / (GLfloat)texW;
		_title.v = h / (GLfloat)texH;
	}
	DecodeBuffer buf;
	memset(&buf, 0, sizeof(buf));
	if (_scalerFactor == 1) {
		buf.w = buf.pitch = texW;
		buf.h = texH;
	} else {
		buf.w = buf.pitch = kW;
		buf.h = kH;
	}
	buf.lut = _tex8to565;
	buf.setPixel = convertTextureTitle;
	buf.ptr = (uint8_t *)calloc(buf.w * buf.h, sizeof(uint16_t));
	if (buf.ptr) {
		res.loadTitleImage(num, &buf);
		glBindTexture(GL_TEXTURE_2D, _title.texId);
		const uint16_t *texData = rescaleTexture(&buf, texW, texH);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW, texH, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, texData);
		free(buf.ptr);
	}
}

static void convertTextureBackground(DecodeBuffer *buf, int x, int y, int w, int h, uint8_t color) {
	const uint16_t *lut = (const uint16_t *)buf->lut;
	const int offset = (y * buf->pitch + x) * sizeof(uint16_t);
	*(uint16_t *)(buf->ptr + offset) = lut[color] | (color >> 7);
}

void TextureCache::createTextureBackground(ResourceData &res, int level, int room) {
	const int w = kW * _scalerFactor;
	const int h = kH * _scalerFactor;
	const int texW = roundPowerOfTwo(w);
	const int texH = roundPowerOfTwo(h);
	if (_background.texId == -1) {
		glGenTextures(1, &_background.texId);
		glBindTexture(GL_TEXTURE_2D, _background.texId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, 0);
		_background.u = w / (GLfloat)texW;
		_background.v = h / (GLfloat)texH;
	}
	if (level != _background.level || room != _background.room) {
		DecodeBuffer buf;
		memset(&buf, 0, sizeof(buf));
		if (_scalerFactor == 1) {
			buf.w = buf.pitch = texW;
			buf.h = texH;
		} else {
			buf.w = buf.pitch = kW;
			buf.h = kH;
		}
		buf.lut = _tex8to5551;
		buf.setPixel = convertTextureBackground;
		buf.ptr = (uint8_t *)calloc(buf.w * buf.h, sizeof(uint16_t));
		if (buf.ptr) {
			res.loadLevelRoom(level, room, &buf);
			glBindTexture(GL_TEXTURE_2D, _background.texId);
			const uint16_t *texData = rescaleTexture(&buf, texW, texH);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW, texH, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, texData);
			free(buf.ptr);
		}
		_background.level = level;
		_background.room = room;
	}
}

static void convertTextureImage(DecodeBuffer *buf, int x, int y, int w, int h, uint8_t color) {
	const uint16_t *lut = (const uint16_t *)buf->lut;
	const int offset = (y * buf->pitch + x) * sizeof(uint16_t);
	*(uint16_t *)(buf->ptr + offset) = lut[color] | 1;
}

void TextureCache::createTextureGfxImage(ResourceData &res, const GfxImage *image) {
	const int w = image->w * _scalerFactor;
	const int h = image->h * _scalerFactor;
	const int texW = roundPowerOfTwo(w);
	const int texH = roundPowerOfTwo(h);
	int pos = -1;
	for (int i = 0; i < ARRAYSIZE(_texturesList); ++i) {
		if (_texturesList[i].texId == -1) {
			pos = i;
		} else if (_texturesList[i].hash.ptr == image->dataPtr && _texturesList[i].hash.num == image->num) {
			queueGfxImageDraw(i, image);
			return;
		}
	}
	if (pos == -1) {
		struct timeval t0;
		gettimeofday(&t0, 0);
		int max = 0;
		for (int i = 0; i < ARRAYSIZE(_texturesList); ++i) {
			if (_texturesList[i].refCount == 0) {
				const int d = (t0.tv_sec - _texturesList[i].t0.tv_sec) * 1000 + (t0.tv_usec - _texturesList[i].t0.tv_usec) / 1000;
				if (i == 0 || d > max) {
					pos = i;
					max = d;
				}
			}
		}
		if (pos == -1) {
			printf("No free slot for texture cache\n");
			return;
		}
		glDeleteTextures(1, &_texturesList[pos].texId);
		memset(&_texturesList[pos], 0, sizeof(Texture));
		_texturesList[pos].texId = -1;
	}
	if (_texturesList[pos].texId == -1) {
		glGenTextures(1, &_texturesList[pos].texId);
		glBindTexture(GL_TEXTURE_2D, _texturesList[pos].texId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, 0);
	}
	_texturesList[pos].u = w / (GLfloat)texW;
	_texturesList[pos].v = h / (GLfloat)texH;
	DecodeBuffer buf;
	memset(&buf, 0, sizeof(buf));
	if (_scalerFactor == 1) {
		buf.w = buf.pitch = texW;
		buf.h = texH;
	} else {
		buf.w = buf.pitch = image->w;
		buf.h = image->h;
	}
	buf.lut = _tex8to5551;
	buf.setPixel = convertTextureImage;
	buf.ptr = (uint8_t *)calloc(buf.w * buf.h, sizeof(uint16_t));
	if (buf.ptr) {
		res.decodeImageData(image->dataPtr, image->num, &buf);
		glBindTexture(GL_TEXTURE_2D, _texturesList[pos].texId);
		const uint16_t *texData = rescaleTexture(&buf, texW, texH);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW, texH, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, texData);
		free(buf.ptr);
	}
	_texturesList[pos].hash.ptr = image->dataPtr;
	_texturesList[pos].hash.num = image->num;
	queueGfxImageDraw(pos, image);
}

void TextureCache::queueGfxImageDraw(int pos, const GfxImage *image) {
	++_texturesList[pos].refCount;
	gettimeofday(&_texturesList[pos].t0, 0);
	assert(_gfxImagesCount < ARRAYSIZE(_gfxImagesQueue));
	_gfxImagesQueue[_gfxImagesCount].x = image->x;
	_gfxImagesQueue[_gfxImagesCount].y = kH - image->y;
	_gfxImagesQueue[_gfxImagesCount].x2 = image->x + image->w;
	_gfxImagesQueue[_gfxImagesCount].y2 = kH - (image->y + image->h);
	_gfxImagesQueue[_gfxImagesCount].xflip = image->xflip;
	_gfxImagesQueue[_gfxImagesCount].erase = image->erase;
	_gfxImagesQueue[_gfxImagesCount].tex = &_texturesList[pos];
	++_gfxImagesCount;
}

void TextureCache::draw(bool menu, int w, int h) {
	if (menu) {
		glColor4f(.7, .7, .7, 1.);
		drawBackground(_title.texId, _title.u, _title.v);
		glColor4f(1., 1., 1., 1.);
		glAlphaFunc(GL_EQUAL, 1);
	} else {
		glAlphaFunc(GL_ALWAYS, 0);
		drawBackground(_background.texId, _background.u, _background.v);
		int i = 0;
		glAlphaFunc(GL_EQUAL, 1);
		for (; i < _gfxImagesCount && !_gfxImagesQueue[i].erase; ++i) {
			drawGfxImage(i);
		}
		drawBackground(_background.texId, _background.u, _background.v);
		for (; i < _gfxImagesCount; ++i) {
			drawGfxImage(i);
		}
		_gfxImagesCount = 0;
	}
}

static void emitQuadTex(int x1, int y1, int x2, int y2, GLfloat u1, GLfloat v1, GLfloat u2, GLfloat v2) {
#ifdef USE_GLES
	const GLfloat textureVertices[] = {
		x1, y2, 0,
		x1, y1, 0,
		x2, y1, 0,
		x2, y2, 0
	};
	const GLfloat textureUv[] = {
		u1, v1,
		u1, v2,
		u2, v2,
		u2, v1
	};
	glVertexPointer(3, GL_FLOAT, 0, textureVertices);
	glTexCoordPointer(2, GL_FLOAT, 0, textureUv);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
#else
	glBegin(GL_QUADS);
		glTexCoord2f(u1, v1);
		glVertex2i(x1, y2);
		glTexCoord2f(u2, v1);
		glVertex2i(x2, y2);
		glTexCoord2f(u2, v2);
		glVertex2i(x2, y1);
		glTexCoord2f(u1, v2);
		glVertex2i(x1, y1);
	glEnd();
#endif
}

void TextureCache::drawBackground(GLuint texId, GLfloat u, GLfloat v) {
	if (texId != -1) {
		glBindTexture(GL_TEXTURE_2D, texId);
		emitQuadTex(0, 0, kW, kH, 0, 0, u, v);
	}
}

void TextureCache::drawGfxImage(int i) {
	Texture *tex = _gfxImagesQueue[i].tex;
	if (tex) {
		glBindTexture(GL_TEXTURE_2D, tex->texId);
		if (_gfxImagesQueue[i].xflip) {
			emitQuadTex(_gfxImagesQueue[i].x2, _gfxImagesQueue[i].y2, _gfxImagesQueue[i].x, _gfxImagesQueue[i].y, 0, 0, tex->u, tex->v);
		} else {
			emitQuadTex(_gfxImagesQueue[i].x, _gfxImagesQueue[i].y2, _gfxImagesQueue[i].x2, _gfxImagesQueue[i].y, 0, 0, tex->u, tex->v);
		}
		--tex->refCount;
	}
}

void TextureCache::drawText(int x, int y, int color, const uint8_t *dataPtr, int len) {
	const uint16_t color5551 = _tex8to5551[color];
	const GLfloat r = ((color5551 >> 11) & 31) / 31.;
	const GLfloat g = ((color5551 >>  6) & 31) / 31.;
	const GLfloat b = ((color5551 >>  1) & 31) / 31.;
	glColor4f(r, g, b, 1.);
	glBindTexture(GL_TEXTURE_2D, _font.texId);
	y = kH - y;
	for (int i = 0; i < len; ++i) {
		const uint8_t code = dataPtr[i];
		const GLfloat texU1 = _font.u * (code - 32) / (GLfloat)_font.charsCount;
		const GLfloat texU2 = _font.u * (code - 31) / (GLfloat)_font.charsCount;
		emitQuadTex(x, y - 16, x + 16, y, texU1, 0., texU2, _font.v);
		x += 16;
	}
	glColor4f(1., 1., 1., 1.);
}
