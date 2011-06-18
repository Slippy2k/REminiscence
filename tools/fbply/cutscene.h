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

#ifndef __CUTSCENE_H__
#define __CUTSCENE_H__

#include "intern.h"

struct SystemStub;

struct Cutscene {
	typedef void (Cutscene::*OpcodeStub)();

	enum {
		SCREEN_W    = 256,
		SCREEN_H    = 224,
		NUM_OPCODES = 15,
		TIMER_SLICE = 15 // XXX experimental
	};

	static const OpcodeStub _opTable[];
	static const char *_namesTable[];
	static const uint16 _offsetsTable[];
	static const uint16 _cosTable[];
	static const uint16 _sinTable[];

	SystemStub *_stub;

	int _inp_keysMask;
	uint8 _palBuf[0x2B * 2];

	Buf _cmdBuf;
	Buf _polBuf;

	Ptr _cmdPtr;

	uint8 *_cmdPtrBak;
	uint8 _interrupted;	
	uint8 _frameDelay;
	uint8 _clearScreen;
	uint8 _palNew;
	uint8 _creditsScene;
	uint8 _cutUnk1;
	uint8 _cutVarText;
	uint8 _cutVarKey;
	
	int16 cut_shape_ix;
	int16 cut_shape_iy;
	int16 cut_shape_ox;
	int16 cut_shape_oy;
	int16 cut_shape_unk_x;
	int16 cut_shape_unk_y;
	int16 cut_shape_unk_x2;	
	int16 cut_shape_unk_y2;
	int16 cut_shape_cur_x;
	int16 cut_shape_cur_y;
	int16 cut_shape_prev_x;
	int16 cut_shape_prev_y;
	uint16 cut_shape_count;
	uint32 cut_shape_cur_x16;
	uint32 cut_shape_cur_y16;
	uint32 cut_shape_prev_x16;
	uint32 cut_shape_prev_y16;
	
	uint32 _rotData[4];
	uint8 _primitiveColor;
	uint16 _startOffset;
	uint16 _cutId;
	const char *_cutName;
	Point _vertices[80];
	uint32 _tstamp;
	const char *_dataDir;

	static void dumpCutNum();

	void readFile(const char *filename, const char *ext, Buf *b);
	void loadPalette();
	void setPalBufColors(const uint8 *palInd, uint16 num);
	void setPalette0xC0();
	void setPalette();
	void blitAndFlip();
	void flipAndVSync();
	void clearScreen(int page);
	void drawText(int16 x, int16 y, uint8 *p, uint16 a, uint8 *page, uint8 mode);
	void initRotData(uint16 a, uint16 b, uint16 c);
	void op_drawShape0Helper(const uint8 *data, int16 x, int16 y); // normal
	void op_drawShape1Helper(uint8 *data, int16 zoom, int16 b, int16 c, int16 d, int16 e, int16 f, int16 g); // zoom
	void op_drawShape2Helper(uint8 *data, int16 zoom, int16 b, int16 c, int16 d, int16 e, int16 f, int16 g); // zoom + rotation

	void op_markCurPos();
	void op_refreshScreen();
	void op_waitForSync();
	void op_drawShape0();
	void op_setPalette();
	void op_drawStringAtBottom();
	void op_nop();
	void op_skip3();
	void op_refreshAll();
	void op_drawShape1();
	void op_drawShape2();
	void op_drawCreditsText();
	void op_drawStringAtPos();
	void op_handleKeys();

	void init(SystemStub *stub, const char *dataDir);
	void start();
	void main(uint16 offset);
	void play();
};

#endif
