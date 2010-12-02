
#include <ctime>
#include "game.h"
#include "util.h"


Game::Game(ResourceData &res)
	: _res(res) {
	_randSeed = time(0);
	_skillLevel = 1;
	_currentLevel = 0;
	_frontLayer = (uint8_t *)calloc(1, kScreenWidth * kScreenHeight);
	_backLayer = (uint8_t *)calloc(1, kScreenWidth * kScreenHeight);
	_inventoryOn = false;
}

#if 0
void Game::run() {
//	_stub->init("REminiscence", kScreenWidth, kScreenHeight);

	_res.load_TEXT();
	_res.load("FB_TXT", Resource::OT_FNT);

	_mix.init();

	playCutscene(0x40);
	playCutscene(0x0D);
	if (!_cut._interrupted) {
		playCutscene(0x4A);
	}

	_res.load("GLOBAL", Resource::OT_ICN);
	_res.load("PERSO", Resource::OT_SPR);
	_res.load_SPR_OFF("PERSO", _res._spr1);
	_res.load_FIB("GLOBAL");

	while (!_pi.quit && _menu.handleTitleScreen(_skillLevel, _currentLevel)) {
		if (_currentLevel == 7) {
			_vid.fadeOut();
			_vid.setTextPalette();
			playCutscene(0x3D);
		} else {
			_vid.setTextPalette();
			_vid.setPalette0xF();
			_stub->setOverscanColor(0xE0);
			mainLoop();
		}
	}

	_res.free_TEXT();

	_mix.free();
	_stub->destroy();
}
#endif

void Game::resetGameState() {
	_animBuffers._states[0] = _animBuffer0State;
	_animBuffers._curPos[0] = 0xFF;
	_animBuffers._states[1] = _animBuffer1State;
	_animBuffers._curPos[1] = 0xFF;
	_animBuffers._states[2] = _animBuffer2State;
	_animBuffers._curPos[2] = 0xFF;
	_animBuffers._states[3] = _animBuffer3State;
	_animBuffers._curPos[3] = 0xFF;
	_currentRoom = _res._pgeInit[0].init_room;
	_cutDeathCutsceneId = 0xFFFF;
	_pge_opTempVar2 = 0xFFFF;
	_deathCutsceneCounter = 0;
	_saveStateCompleted = false;
	_loadMap = true;
	pge_resetGroups();
	_blinkingConradCounter = 0;
	_pge_processOBJ = false;
	_pge_opTempVar1 = 0;
	_textToDisplay = 0xFFFF;
	_textSegment = 0;
}

void Game::initGame() {
//	_vid._unkPalSlot1 = 0;
//	_vid._unkPalSlot2 = 0;
	_score = 0;
	loadLevelData();
	resetGameState();
}

void Game::doTick() {
	doStoryTexts();
#if 0
		playCutscene();
		if (_cutId == 0x3D) {
			showFinalScore();
			break;
		}
#endif
		if (_deathCutsceneCounter) {
			--_deathCutsceneCounter;
			if (_deathCutsceneCounter == 0) {
#if 0
				playCutscene(_cutDeathCutsceneId);
				if (!handleContinueAbort()) {
					playCutscene(0x41);
					break;
				} else {
					if (_validSaveState) {
						if (!loadGameState(0)) {
							break;
						}
					} else {
						loadLevelData();
						resetGameState();
					}
					continue;
				}
#endif
			}
		}
		memcpy(_frontLayer, _backLayer, kScreenWidth * kScreenHeight);
		pge_getInput();
		pge_prepare();
		col_prepareRoomState();
		uint8 oldLevel = _currentLevel;
		for (uint16 i = 0; i < _res._pgeNum; ++i) {
			LivePGE *pge = _pge_liveTable2[i];
			if (pge) {
				_col_currentPiegeGridPosY = (pge->pos_y / 36) & ~1;
				_col_currentPiegeGridPosX = (pge->pos_x + 8) >> 4;
				pge_process(pge);
			}
		}
		if (oldLevel != _currentLevel) {
assert(0);
			changeLevel();
			_pge_opTempVar1 = 0;
			return;
		}
		if (_loadMap) {
			if (_currentRoom == 0xFF) {
assert(0);
				_cutId = 6;
				_deathCutsceneCounter = 1;
			} else {
				_currentRoom = _pgeLive[0].room_location;
				loadLevelMap();
				_loadMap = false;
//				_vid.fullRefresh();
			}
		}
		prepareAnims();
		drawAnims();
		drawCurrentInventoryItem();
		drawLevelTexts();
		printLevelCode();
		if (_blinkingConradCounter != 0) {
			--_blinkingConradCounter;
		}
}

void Game::playCutscene(int id) {
#if 0
	if (id != -1) {
		_cutId = id;
	}
	if (_cutId != 0xFFFF) {
		_sfxPly.stop();
		_cut.play();
	}
#endif
}

void Game::drawCurrentInventoryItem() {
	uint16 src = _pgeLive[0].current_inventory_PGE;
	if (src != 0xFF) {
		_currentIcon = _res._pgeInit[src].icon_num;
		drawIcon(_currentIcon, 232, 8, 0xA);
	}
}

#if 0
bool Game::handleConfigPanel() {
	int i, j;
	const int x = 7;
	const int y = 10;
	const int w = 17;
	const int h = 12;

	_vid._charShadowColor = 0xE2;
	_vid._charFrontColor = 0xEE;
	_vid._charTransparentColor = 0xFF;

	_vid.drawChar(0x81, y, x);
	for (i = 1; i < w; ++i) {
		_vid.drawChar(0x85, y, x + i);
	}
	_vid.drawChar(0x82, y, x + w);
	for (j = 1; j < h; ++j) {
		_vid.drawChar(0x86, y + j, x);
		for (i = 1; i < w; ++i) {
			_vid._charTransparentColor = 0xE2;
			_vid.drawChar(0x20, y + j, x + i);
		}
		_vid._charTransparentColor = 0xFF;
		_vid.drawChar(0x87, y + j, x + w);
	}
	_vid.drawChar(0x83, y + h, x);
	for (i = 1; i < w; ++i) {
		_vid.drawChar(0x88, y + h, x + i);
	}
	_vid.drawChar(0x84, y + h, x + w);

	_menu._charVar3 = 0xE4;
	_menu._charVar4 = 0xE5;
	_menu._charVar1 = 0xE2;
	_menu._charVar2 = 0xEE;

	_vid.fullRefresh();
	enum { MENU_ITEM_LOAD = 1, MENU_ITEM_SAVE = 2, MENU_ITEM_ABORT = 3 };
	uint8 colors[] = { 2, 3, 3, 3 };
	int current = 0;
	while (!_pi.quit) {
		_menu.drawString(_res.getMenuString(LocaleData::LI_18_RESUME_GAME), y + 2, 9, colors[0]);
		_menu.drawString(_res.getMenuString(LocaleData::LI_20_LOAD_GAME), y + 4, 9, colors[1]);
		_menu.drawString(_res.getMenuString(LocaleData::LI_21_SAVE_GAME), y + 6, 9, colors[2]);
		_menu.drawString(_res.getMenuString(LocaleData::LI_19_ABORT_GAME), y + 8, 9, colors[3]);
		char slotItem[30];
		sprintf(slotItem, "%s : %d-%02d", _res.getMenuString(LocaleData::LI_22_SAVE_SLOT), _currentLevel + 1, _stateSlot);
		_menu.drawString(slotItem, y + 10, 9, 1);

		_vid.updateScreen();
		_stub->sleep(80);
		inp_update();

		int prev = current;
		if (_pi.dirMask & PlayerInput::kDirectionUp) {
			_pi.dirMask &= ~PlayerInput::kDirectionUp;
			current = (current + 3) % 4;
		}
		if (_pi.dirMask & PlayerInput::kDirectionDown) {
			_pi.dirMask &= ~PlayerInput::kDirectionDown;
			current = (current + 1) % 4;
		}
		if (_pi.dirMask & PlayerInput::kDirectionLeft) {
			_pi.dirMask &= ~PlayerInput::kDirectionLeft;
			--_stateSlot;
			if (_stateSlot < 1) {
				_stateSlot = 1;
			}
		}
		if (_pi.dirMask & PlayerInput::kDirectionRight) {
			_pi.dirMask &= ~PlayerInput::kDirectionRight;
			++_stateSlot;
			if (_stateSlot > 99) {
				_stateSlot = 99;
			}
		}
		if (prev != current) {
			SWAP(colors[prev], colors[current]);
		}
		if (_pi.enter) {
			_pi.enter = false;
			switch (current) {
			case MENU_ITEM_LOAD:
				_pi.load = true;
				break;
			case MENU_ITEM_SAVE:
				_pi.save = true;
				break;
			}
			break;
		}
	}
	_vid.fullRefresh();
	return (current == MENU_ITEM_ABORT);
}

bool Game::handleContinueAbort() {
	playCutscene(0x48);
	char textBuf[50];
	int timeout = 100;
	int current_color = 0;
	uint8 colors[] = { 0xE4, 0xE5 };
	uint8 color_inc = 0xFF;
	Color col;
	_stub->getPaletteEntry(0xE4, &col);
	memcpy(_vid._tempLayer, _vid._frontLayer, kScreenWidth * kScreenHeight);
	while (timeout >= 0 && !_pi.quit) {
		const char *str;
		str = _res.getMenuString(LocaleData::LI_01_CONTINUE_OR_ABORT);
		_vid.drawString(str, (256 - strlen(str) * 8) / 2, 64, 0xE3);
		str = _res.getMenuString(LocaleData::LI_02_TIME);
		sprintf(textBuf, "%s : %d", str, timeout / 10);
		_vid.drawString(textBuf, 96, 88, 0xE3);
		str = _res.getMenuString(LocaleData::LI_03_CONTINUE);
		_vid.drawString(str, (256 - strlen(str) * 8) / 2, 104, colors[0]);
		str = _res.getMenuString(LocaleData::LI_04_ABORT);
		_vid.drawString(str, (256 - strlen(str) * 8) / 2, 112, colors[1]);
		sprintf(textBuf, "SCORE  %08u", _score);
		_vid.drawString(textBuf, 64, 154, 0xE3);
		if (_pi.dirMask & PlayerInput::kDirectionUp) {
			_pi.dirMask &= ~PlayerInput::kDirectionUp;
			if (current_color > 0) {
				SWAP(colors[current_color], colors[current_color - 1]);
				--current_color;
			}
		}
		if (_pi.dirMask & PlayerInput::kDirectionDown) {
			_pi.dirMask &= ~PlayerInput::kDirectionDown;
			if (current_color < 1) {
				SWAP(colors[current_color], colors[current_color + 1]);
				++current_color;
			}
		}
		if (_pi.enter) {
			_pi.enter = false;
			return (current_color == 0);
		}
		_stub->copyRect(0, 0, kScreenWidth, kScreenHeight, _vid._frontLayer, 256);
		_stub->updateScreen(0);
		if (col.b >= 0x3D) {
			color_inc = 0;
		}
		if (col.b < 2) {
			color_inc = 0xFF;
		}
		if (color_inc == 0xFF) {
			col.b += 2;
			col.g += 2;
		} else {
			col.b -= 2;
			col.g -= 2;
		}
		_stub->setPaletteEntry(0xE4, &col);
		_stub->processEvents();
		_stub->sleep(100);
		--timeout;
		memcpy(_vid._frontLayer, _vid._tempLayer, kScreenWidth * kScreenHeight);
	}
	return false;
}
#endif

void Game::printLevelCode() {
	if (_printLevelCodeCounter != 0) {
		--_printLevelCodeCounter;
		if (_printLevelCodeCounter != 0) {
#if 0
			char buf[50];
			snprintf(buf, sizeof(buf), "CODE: %s", _menu._passwords[_currentLevel][_skillLevel]);
			drawString(buf, (kScreenWidth - strlen(buf) * 8) / 2, 16, 0xE7);
#endif
		}
	}
}

void Game::printSaveStateCompleted() {
#if 0
	if (_saveStateCompleted) {
		const char *str = _res.getMenuString(LocaleData::LI_05_COMPLETED);
		_vid.drawString(str, (176 - strlen(str) * 8) / 2, 34, 0xE6);
	}
#endif
}

void Game::drawLevelTexts() {
	LivePGE *pge = &_pgeLive[0];
	int8 obj = col_findCurrentCollidingObject(pge, 3, 0xFF, 0xFF, &pge);
	if (obj == 0) {
		obj = col_findCurrentCollidingObject(pge, 0xFF, 5, 9, &pge);
	}
	if (obj > 0) {
		_printLevelCodeCounter = 0;
		if (_textToDisplay == 0xFFFF) {
			uint8 icon_num = obj - 1;
			drawIcon(icon_num, 80, 8, 0xA);
			const uint8_t *str = _res.getStringData(pge->init_PGE->text_num);
			drawString(str + 1, *str, (176 - *str * 8) / 2, 26, 0xE6);
			if (icon_num == 2) {
				printSaveStateCompleted();
				return;
			}
		} else {
			_currentInventoryIconNum = obj - 1;
		}
	}
	_saveStateCompleted = false;
}

void Game::doStoryTexts() {
	// TODO: clear background
	if (_textToDisplay != 0xFFFF) {
		const uint8_t *str = _res.getStringData(ResourceData::kGameString + _textToDisplay);
		const int segmentsCount = *str++;
printf("segmentsCount %d _textSegment %d\n", segmentsCount, _textSegment);
		// goto to current segment
		for (int i = 0; i < _textSegment; ++i) {
			const int segmentLength = *str++;
			str += segmentLength;
		}
		int len = *str++;
		int color = 0xE8;
		if (*str == '@') {
			switch (*str++) {
			case '1':
				color = 0xE9;
				break;
			case '2':
				color = 0xEB;
				break;
			default:
				assert(0);
				break;
			}
			++str;
			len -= 2;
		}
		drawIcon(_currentInventoryIconNum, 80, 8, 0xA);
		drawString(str, len, (176 - len * 8) / 2, 26, color);
		if (_pi.backspace) {
			_pi.backspace = false;
			++_textSegment;
			if (_textSegment == segmentsCount) {
				_textSegment = 0;
				_textToDisplay = 0xFFFF;
			}
		}
	}
}

void Game::prepareAnims() {
	if (!(_currentRoom & 0x80) && _currentRoom < 0x40) {
		int8 pge_room;
		LivePGE *pge = _pge_liveTable1[_currentRoom];
		while (pge) {
			prepareAnimsHelper(pge, 0, 0);
			pge = pge->next_PGE_in_room;
		}
		pge_room = _res._ctData[kCtRoomTop + _currentRoom];
		if (pge_room >= 0 && pge_room < 0x40) {
			pge = _pge_liveTable1[pge_room];
			while (pge) {
				if ((pge->init_PGE->object_type != 10 && pge->pos_y > 176) || (pge->init_PGE->object_type == 10 && pge->pos_y > 216)) {
					prepareAnimsHelper(pge, 0, -216);
				}
				pge = pge->next_PGE_in_room;
			}
		}
		pge_room = _res._ctData[kCtRoomBottom + _currentRoom];
		if (pge_room >= 0 && pge_room < 0x40) {
			pge = _pge_liveTable1[pge_room];
			while (pge) {
				if (pge->pos_y < 48) {
					prepareAnimsHelper(pge, 0, 216);
				}
				pge = pge->next_PGE_in_room;
			}
		}
		pge_room = _res._ctData[kCtRoomLeft + _currentRoom];
		if (pge_room >= 0 && pge_room < 0x40) {
			pge = _pge_liveTable1[pge_room];
			while (pge) {
				if (pge->pos_x > 224) {
					prepareAnimsHelper(pge, -256, 0);
				}
				pge = pge->next_PGE_in_room;
			}
		}
		pge_room = _res._ctData[kCtRoomRight + _currentRoom];
		if (pge_room >= 0 && pge_room < 0x40) {
			pge = _pge_liveTable1[pge_room];
			while (pge) {
				if (pge->pos_x <= 32) {
					prepareAnimsHelper(pge, 256, 0);
				}
				pge = pge->next_PGE_in_room;
			}
		}
	}
}

void Game::prepareAnimsHelper(LivePGE *pge, int16 dx, int16 dy) {
	debug(DBG_GAME, "Game::prepareAnimsHelper() dx=0x%X dy=0x%X pge_num=%d pge->flags=0x%X pge->anim_number=0x%X", dx, dy, pge - &_pgeLive[0], pge->flags, pge->anim_number);
	int16 xpos, ypos;
	if (!(pge->flags & 8)) {
		if (pge->index != 0 && loadMonsterSprites(pge) == 0) {
			return;
		}
		assert(pge->anim_number < 1287);
		const uint8 *dataPtr = 0; //_res.getImageData(_res._perso, _res.getSpriteFrame(pge->anim_number));
		xpos = dx + pge->pos_x;
		ypos = dy + pge->pos_y + 2;
		if (xpos <= -32 || xpos >= 256 || ypos < -48 || ypos >= 224) {
			return;
		}
		xpos += 8;
		dataPtr += 4;
		if (pge == &_pgeLive[0]) {
			_animBuffers.addState(1, xpos, ypos, dataPtr, pge);
		} else if (pge->flags & 0x10) {
			_animBuffers.addState(2, xpos, ypos, dataPtr, pge);
		} else {
			_animBuffers.addState(0, xpos, ypos, dataPtr, pge);
		}
	} else {
		const uint8_t *dataPtr = 0;
		xpos = dx + pge->pos_x + 8;
		ypos = dy + pge->pos_y + 2;

		if (pge->init_PGE->object_type == 11) {
			_animBuffers.addState(3, xpos, ypos, dataPtr, pge);
		} else if (pge->flags & 0x10) {
			_animBuffers.addState(2, xpos, ypos, dataPtr, pge);
		} else {
			_animBuffers.addState(0, xpos, ypos, dataPtr, pge);
		}
	}
}

void Game::drawAnims() {
	debug(DBG_GAME, "Game::drawAnims()");
	_eraseBackground = false;
	drawAnimBuffer(2, _animBuffer2State);
	drawAnimBuffer(1, _animBuffer1State);
	drawAnimBuffer(0, _animBuffer0State);
	_eraseBackground = true;
	drawAnimBuffer(3, _animBuffer3State);
}

static void initDecodeBuffer(DecodeBuffer *buf, int x, int y, bool xflip, bool erase, uint8 *dstPtr, const uint8_t *dataPtr) {
	memset(buf, 0, sizeof(DecodeBuffer));
	buf->ptr = dstPtr;
	buf->w = buf->pitch = Game::kScreenWidth;
	buf->h = Game::kScreenHeight;
	buf->xflip = xflip;
	buf->yflip = false;
	buf->x = x * 2;
	buf->y = y * 2;
	if (dataPtr) {
		if (xflip) {
			buf->x += (int16)READ_BE_UINT16(dataPtr + 4) - READ_BE_UINT16(dataPtr) - 1;
		} else {
			buf->x -= (int16)READ_BE_UINT16(dataPtr + 4);
		}
		buf->y -= (int16)READ_BE_UINT16(dataPtr + 6);
	}
	buf->erase = erase;
}

void Game::drawPiege(LivePGE *pge, int x, int y) {
	// TODO: factorize
	DecodeBuffer buf;
	if (pge->flags & 8) {
		const uint8_t *dataPtr = _res.getImageData(_res._spc, pge->anim_number);
if (!dataPtr) return;
		initDecodeBuffer(&buf, x, y, false, _eraseBackground,  _frontLayer, dataPtr);
		_res.decodeImageData(_res._spc, pge->anim_number, &buf);
	} else {
		const bool xflip = (pge->flags & 2);
		if (pge->index == 0) {
			const int frame = _res.getPersoFrame(pge->anim_number);
			const uint8_t *dataPtr = _res.getImageData(_res._perso, frame);
if (!dataPtr) return;
			initDecodeBuffer(&buf, x, y, xflip, _eraseBackground, _frontLayer, dataPtr);
			_res.decodeImageData(_res._perso, frame, &buf);
		} else {
			const int frame = _res.getMonsterFrame(pge->anim_number);
			const uint8_t *dataPtr = _res.getImageData(_res._monster, frame);
if (!dataPtr) return;
			initDecodeBuffer(&buf, x, y, xflip, _eraseBackground, _frontLayer, dataPtr);
			_res.decodeImageData(_res._monster, frame, &buf);
		}
	}
}

void Game::drawString(const unsigned char *str, int len, int x, int y, int color) {
	int offset = 0;
	for (int i = 0; i < len; ++i) {
		const uint8_t code = *str++;
		if (code == 0x7C) {
			y += 8;
			offset = 0;
			continue;
		}
		DecodeBuffer buf;
		initDecodeBuffer(&buf, x + offset, y, false, true, _frontLayer, 0);
		buf.textColor = color;
		_res.decodeImageData(_res._fnt, code - 32, &buf);
		offset += 8;
	}
}

void Game::drawAnimBuffer(uint8 stateNum, AnimBufferState *state) {
	debug(DBG_GAME, "Game::drawAnimBuffer() state=%d", stateNum);
	assert(stateNum < 4);
	_animBuffers._states[stateNum] = state;
	const uint8 lastPos = _animBuffers._curPos[stateNum];
	if (lastPos != 0xFF) {
		uint8 numAnims = lastPos + 1;
		state += lastPos;
		_animBuffers._curPos[stateNum] = 0xFF;
		do {
			LivePGE *pge = state->pge;
			if (!(pge->flags & 8)) {
				if (stateNum == 1 && (_blinkingConradCounter & 1)) {
					break;
				}
			}
			drawPiege(pge, state->x, state->y);
			--state;
		} while (--numAnims != 0);
	}
}

int Game::loadMonsterSprites(LivePGE *pge) {
	debug(DBG_GAME, "Game::loadMonsterSprites()");
	InitPGE *init_pge = pge->init_PGE;
	if (init_pge->obj_node_number != 0x49 && init_pge->object_type != 10) {
		return 0xFFFF;
	}
	if (init_pge->obj_node_number == _curMonsterFrame) {
		return 0xFFFF;
	}
	if (pge->room_location != _currentRoom) {
		return 0;
	}

	const uint8 *mList = _monsterListLevels[_currentLevel];
	while (*mList != init_pge->obj_node_number) {
		if (*mList == 0xFF) { // end of list
			return 0;
		}
		mList += 2;
	}
	_curMonsterFrame = mList[0];
	if (_curMonsterNum != mList[1]) {
		_curMonsterNum = mList[1];
		_res.loadMonsterData(_monsterNames[_curMonsterNum], _palette);
	}
	return 0xFFFF;
}

void Game::loadLevelMap() {
	debug(DBG_GAME, "Game::loadLevelMap() room=%d", _currentRoom);
	_currentIcon = 0xFF;
	DecodeBuffer buf;
	initDecodeBuffer(&buf, 0, 0, false, true, _frontLayer, 0);
	_res.loadLevelRoom(_currentLevel, _currentRoom, &buf);
	memcpy(_backLayer, _frontLayer, kScreenWidth * kScreenHeight);
	_res.setupLevelClut(_currentLevel, _palette);
}

void Game::loadLevelData() {
#if 0
	_cutId = lvl->cutscene_id;
#endif

	_res.loadLevelData(_currentLevel);
	_res.loadLevelObjects(_currentLevel);

	_curMonsterNum = 0xFFFF;
	_curMonsterFrame = 0;

	_printLevelCodeCounter = 150;

	_col_slots2Cur = _col_slots2;
	_col_slots2Next = 0;

	memset(_pge_liveTable2, 0, sizeof(_pge_liveTable2));
	memset(_pge_liveTable1, 0, sizeof(_pge_liveTable1));

	_currentRoom = _res._pgeInit[0].init_room;
	uint16 n = _res._pgeNum;
	while (n--) {
		pge_loadForCurrentLevel(n);
	}
	for (uint16 i = 0; i < _res._pgeNum; ++i) {
		if (_res._pgeInit[i].skill <= _skillLevel) {
			LivePGE *pge = &_pgeLive[i];
			pge->next_PGE_in_room = _pge_liveTable1[pge->room_location];
			_pge_liveTable1[pge->room_location] = pge;
		}
	}
	pge_resetGroups();
//	_validSaveState = false;
}

void Game::drawIcon(uint8 iconNum, int16 x, int16 y, uint8 colMask) {
	DecodeBuffer buf;
	initDecodeBuffer(&buf, x, y, false, true, _frontLayer, 0);
	_res.decodeImageData(_res._icn, iconNum, &buf);
}

void Game::playSound(uint8 sfxId, uint8 softVol) {
#if 0
	if (sfxId < _res._numSfx) {
		SoundFx *sfx = &_res._sfxList[sfxId];
		if (sfx->data) {
			MixerChunk mc;
			mc.data = sfx->data;
			mc.len = sfx->len;
			const int freq = _res._resType == Resource::kResourceTypeAmiga ? 3546897 / 650 : 6000;
			_mix.play(&mc, freq, Mixer::MAX_VOLUME >> softVol);
		}
	} else {
		// in-game music
		_sfxPly.play(sfxId);
	}
#endif
}

uint16 Game::getRandomNumber() {
	uint32 n = _randSeed * 2;
	if (_randSeed > n) {
		n ^= 0x1D872B41;
	}
	_randSeed = n;
	return n & 0xFFFF;
}

void Game::changeLevel() {
//	_vid.fadeOut();
	loadLevelData();
	loadLevelMap();
}

uint16 Game::getLineLength(const uint8 *str) const {
	uint16 len = 0;
	while (*str && *str != 0xB && *str != 0xA) {
		++str;
		++len;
	}
	return len;
}

void Game::initInventory() {
	if (!_inventoryOn) {
		_inventoryCurrentItem = 0;
	} else {
		pge_setCurrentInventoryObject(_inventoryItems[_inventoryCurrentItem].live_pge);
	}
	_inventoryOn = !_inventoryOn;
}

void Game::doInventory() {
	LivePGE *pge = &_pgeLive[0];
	if (pge->life > 0 && pge->current_inventory_PGE != 0xFF) {
//		playSound(66, 0);
		int itemsCount = 0;
		int i = pge->current_inventory_PGE;
		while (i != 0xFF) {
			_inventoryItems[itemsCount].icon_num = _res._pgeInit[i].icon_num;
			_inventoryItems[itemsCount].init_pge = &_res._pgeInit[i];
			_inventoryItems[itemsCount].live_pge = &_pgeLive[i];
			i = _pgeLive[i].next_inventory_PGE;
			++itemsCount;
		}
		_inventoryItems[itemsCount].icon_num = 0xFF;

		drawIcon(31, 56, 140, 0xF);
		const int h = (itemsCount - 1) / 4 + 1;
		int y = _inventoryCurrentItem / 4;

		for (int i = 0; i < 4; ++i) {
			int currentItem = y * 4 + i;
			if (_inventoryItems[currentItem].icon_num == 0xFF) {
				break;
			}
			const int xPos = 72 + i * 32;
			drawIcon(_inventoryItems[currentItem].icon_num, xPos, 157, 0xA);
			if (_inventoryCurrentItem == currentItem) {
				drawIcon(32, xPos, 157, 0xA);
				const LivePGE *selected_pge = _inventoryItems[currentItem].live_pge;
				const uint8_t *str = _res.getStringData(_inventoryItems[currentItem].init_pge->text_num);
				assert(str);
				drawString(str + 1, *str, (256 - *str * 8) / 2, 189, 0xED);
				if (_inventoryItems[currentItem].init_pge->init_flags & 4) {
					uint8_t buf[10];
					const int len = snprintf((char *)buf, sizeof(buf), "%d", selected_pge->life);
					drawString(buf, len, (256 - len * 8) / 2, 197, 0xED);
				}
			}
		}
		if (y != 0) {
			drawIcon(34, 120, 176, 0xA); // down arrow
		}
		if (y != h - 1) {
			drawIcon(33, 120, 143, 0xA); // up arrow
		}
		if (_pi.dirMask & PlayerInput::kDirectionUp) {
			_pi.dirMask &= ~PlayerInput::kDirectionUp;
			if (y < h - 1) {
				++y;
				_inventoryCurrentItem = y * 4;
			}
		}
		if (_pi.dirMask & PlayerInput::kDirectionDown) {
			_pi.dirMask &= ~PlayerInput::kDirectionDown;
			if (y > 0) {
				--y;
				_inventoryCurrentItem = y * 4;
			}
		}
		if (_pi.dirMask & PlayerInput::kDirectionLeft) {
			_pi.dirMask &= ~PlayerInput::kDirectionLeft;
			if (_inventoryCurrentItem > 0) {
				const int num = _inventoryCurrentItem % 4;
				if (num > 0) {
					--_inventoryCurrentItem;
				}
			}
		}
		if (_pi.dirMask & PlayerInput::kDirectionRight) {
			_pi.dirMask &= ~PlayerInput::kDirectionRight;
			if (_inventoryCurrentItem < itemsCount - 1) {
				const int num = _inventoryCurrentItem % 4;
				if (num < 3) {
					++_inventoryCurrentItem;
				}
			}
		}
//		playSound(66, 0);
	}
}

void AnimBuffers::addState(uint8 stateNum, int16 x, int16 y, const uint8 *dataPtr, LivePGE *pge) {
	debug(DBG_GAME, "AnimBuffers::addState() stateNum=%d x=%d y=%d dataPtr=0x%X pge=0x%X", stateNum, x, y, dataPtr, pge);
	assert(stateNum < 4);
	AnimBufferState *state = _states[stateNum];
	state->x = x;
	state->y = y;
	state->dataPtr = dataPtr;
	state->pge = pge;
	++_curPos[stateNum];
	++_states[stateNum];
}

