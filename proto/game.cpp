
#include <time.h>
#include "game.h"
#include "util.h"


Game::Game(ResourceData &res)
	: _res(res) {
	_randSeed = time(0);
	_skillLevel = 1;
	_currentLevel = 0;
	_frontLayer = (uint8_t *)calloc(1, kScreenWidth * kScreenHeight);
	_backLayer = (uint8_t *)calloc(1, kScreenWidth * kScreenHeight);
	_tempLayer = 0;
	_inventoryOn = false;
	_hotspotsCount = 0;
	_shakeOffset = 0;
}

Game::~Game() {
	free(_frontLayer);
	free(_backLayer);
	free(_tempLayer);
}

#if 0
void Game::run() {
	playCutscene(0x40);
	playCutscene(0x0D);
	if (!_cut._interrupted) {
		playCutscene(0x4A);
	}
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
	_nextTextSegment = 0;
}

void Game::initGame() {
//	_vid._unkPalSlot1 = 0;
//	_vid._unkPalSlot2 = 0;
	_score = 0;
	loadLevelData();
	resetGameState();
}

void Game::doHotspots() {
	if (_pi.touch.press != PlayerInput::kTouchNone) {
		for (int i = 0; i < _hotspotsCount; ++i) {
			Hotspot *hs = &_hotspotsList[i];
			if (hs->testPos(_pi.touch.x, _pi.touch.y)) {
				switch (hs->id) {
				case Hotspot::kIdUseGun:
					_pi.space = (_pi.touch.press == PlayerInput::kTouchDown);
					break;
				case Hotspot::kIdUseInventory:
					if (_pi.touch.press == PlayerInput::kTouchUp) {
						_pi.backspace = true;
					}
					break;
				case Hotspot::kIdSelectInventoryObject:
					if (_pi.touch.press == PlayerInput::kTouchDown) {
						const int num = (hs->x / kHotspotCoordScale - 72) / 32;
						if (num >= 0 && num <= 3) {
							_inventoryCurrentItem = (_inventoryCurrentItem & ~3) | num;
						}
					}
					break;
				case Hotspot::kIdScrollDownInventory:
					if (_pi.touch.press == PlayerInput::kTouchUp) {
						if (_inventoryCurrentItem - 4 >= 0) {
							_inventoryCurrentItem = (_inventoryCurrentItem - 4) & ~3;
						}
					}
					break;
				case Hotspot::kIdScrollUpInventory:
					if (_pi.touch.press == PlayerInput::kTouchUp) {
						if (((_inventoryCurrentItem + 4) & ~3) < _inventoryItemsCount) {
							_inventoryCurrentItem = (_inventoryCurrentItem + 4) & ~3;
						}
					}
					break;
				}
			}
		}
	}
}

void Game::doTick() {
	_paletteChanged = false;
	_backgroundChanged = false;
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
				_cutId = 6;
				_deathCutsceneCounter = 1;
			} else {
				_currentRoom = _pgeLive[0].room_location;
				loadLevelMap();
				_loadMap = false;
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

void Game::drawHotspots() {
	if (_pi.touch.press == PlayerInput::kTouchDown) {
		for (int i = 0; i < _hotspotsCount; ++i) {
			Hotspot *hs = &_hotspotsList[i];
			if (hs->testPos(_pi.touch.x, _pi.touch.y)) {
				switch (hs->id) {
				case Hotspot::kIdUseGun:
				case Hotspot::kIdUseInventory:
					drawIcon(32, hs->x / kHotspotCoordScale, hs->y / kHotspotCoordScale, 0xA);
					break;
				}
			}
		}
	}
}

void Game::playCutscene(int id) {
	if (id != -1) {
		_cutId = id;
	}
	if (_cutId != 0xFFFF) {
#if 0
		_sfxPly.stop();
		_cut.play();
#endif
	}
}

void Game::drawCurrentInventoryItem() {
	int src = _pgeLive[0].current_inventory_PGE;
	if (src != 0xFF) {
		drawIcon(_res._pgeInit[src].icon_num, 232, 8, 0xA);
		addHotspot(Hotspot::kIdUseInventory, 232, 8, 16, 16);
		while (src != 0xFF) {
			if (_res._pgeInit[src].object_id == kGunObject) {
				drawIcon(_res._pgeInit[src].icon_num, 208, 8, 0xA);
				addHotspot(Hotspot::kIdUseGun, 208, 8, 16, 16);
				break;
			}
			src = _pgeLive[src].next_inventory_PGE;
		}
	}
}

void Game::printLevelCode() {
	if (_printLevelCodeCounter != 0) {
		--_printLevelCodeCounter;
		if (_printLevelCodeCounter != 0) {
		}
	}
}

void Game::printSaveStateCompleted() {
	if (_saveStateCompleted) {
		const char *str = "COMPLETED";
		const int len = strlen(str);
		drawString((const uint8_t *)str, len, (176 - len * 8) / 2, 34, 0xE6);
	}
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
			const uint8_t icon_num = obj - 1;
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
	if (!_tempLayer) {
		_tempLayer = (uint8_t *)malloc(kScreenWidth * kScreenHeight);
		if (_tempLayer) {
			memcpy(_tempLayer, _frontLayer, kScreenWidth * kScreenHeight);
		}
	}
	const uint8_t *str = _res.getStringData(ResourceData::kGameString + _textToDisplay);
	const int segmentsCount = *str++;
	if (_pi.backspace) {
		_pi.backspace = false;
		if (_nextTextSegment >= segmentsCount) {
			_nextTextSegment = 0;
			_textToDisplay = 0xFFFF;
			free(_tempLayer);
			_tempLayer = 0;
			return;
                }
		if (_tempLayer) {
			memcpy(_frontLayer, _tempLayer, kScreenWidth * kScreenHeight);
		}
	} else if (_nextTextSegment > 0) {
		return;
	}
		// goto to current segment
		for (int i = 0; i < _nextTextSegment; ++i) {
			const int segmentLength = *str++;
			str += segmentLength;
		}
		int len = *str++;
		int color = 0xE8;
		if (*str == '@') {
			++str;
			switch (*str) {
			case '1':
				color = 0xE9;
				break;
			case '2':
				color = 0xEB;
				break;
			}
			++str;
			len -= 2;
		}
		drawIcon(_currentInventoryIconNum, 80, 8, 0xA);
		int lineOffset = 26;
		while (len > 0) {
			const uint8_t *next = (const uint8_t *)memchr(str, 0x7C, len);
			if (!next) {
				drawString(str, len, (176 - len * 8) / 2, lineOffset, color);
				break;
			}
			const int lineLength = next - str;
			drawString(str, lineLength, (176 - lineLength * 8) / 2, lineOffset, color);
			str = next + 1;
			len -= lineLength + 1;
			lineOffset += 8;
		}
	++_nextTextSegment;
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
		xpos = dx + pge->pos_x;
		ypos = dy + pge->pos_y + 2;
		if (xpos <= -32 || xpos >= 256 || ypos < -48 || ypos >= 224) {
			return;
		}
		xpos += 8;
		if (pge == &_pgeLive[0]) {
			_animBuffers.addState(1, xpos, ypos, pge);
		} else if (pge->flags & 0x10) {
			_animBuffers.addState(2, xpos, ypos, pge);
		} else {
			_animBuffers.addState(0, xpos, ypos, pge);
		}
	} else {
		xpos = dx + pge->pos_x + 8;
		ypos = dy + pge->pos_y + 2;
		if (pge->init_PGE->object_type == 11) {
			_animBuffers.addState(3, xpos, ypos, pge);
		} else if (pge->flags & 0x10) {
			_animBuffers.addState(2, xpos, ypos, pge);
		} else {
			_animBuffers.addState(0, xpos, ypos, pge);
		}
	}
}

void Game::drawAnims() {
	debug(DBG_GAME, "Game::drawAnims()");
	_eraseBackground = false; // enable stencil test
	drawAnimBuffer(2, _animBuffer2State);
	drawAnimBuffer(1, _animBuffer1State);
	drawAnimBuffer(0, _animBuffer0State);
	_eraseBackground = true; // disable stencil test
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
	DecodeBuffer buf;
	if (pge->flags & 8) {
		const uint8_t *dataPtr = _res.getImageData(_res._spc, pge->anim_number);
		if (!dataPtr) return;
		initDecodeBuffer(&buf, x, y, false, _eraseBackground,  _frontLayer, dataPtr);
		_res.decodeImageData(_res._spc, pge->anim_number, &buf);
		addImageToList(buf.x, buf.y, READ_BE_UINT16(dataPtr), READ_BE_UINT16(dataPtr + 2), false, _eraseBackground, _res._spc, pge->anim_number);
	} else {
		const bool xflip = (pge->flags & 2);
		if (pge->index == 0) {
			const int frame = _res.getPersoFrame(pge->anim_number);
			const uint8_t *dataPtr = _res.getImageData(_res._perso, frame);
			if (!dataPtr) return;
			initDecodeBuffer(&buf, x, y, xflip, _eraseBackground, _frontLayer, dataPtr);
			_res.decodeImageData(_res._perso, frame, &buf);
			addImageToList(buf.x, buf.y, READ_BE_UINT16(dataPtr), READ_BE_UINT16(dataPtr + 2), xflip, _eraseBackground, _res._perso, frame);
		} else {
			const int frame = _res.getMonsterFrame(pge->anim_number);
			const uint8_t *dataPtr = _res.getImageData(_res._monster, frame);
			if (!dataPtr) return;
			initDecodeBuffer(&buf, x, y, xflip, _eraseBackground, _frontLayer, dataPtr);
			_res.decodeImageData(_res._monster, frame, &buf);
			addImageToList(buf.x, buf.y, READ_BE_UINT16(dataPtr), READ_BE_UINT16(dataPtr + 2), xflip, _eraseBackground, _res._monster, frame);
		}
	}
}

void Game::drawString(const unsigned char *str, int len, int x, int y, int color) {
	int offset = 0;
	for (int i = 0; i < len; ++i) {
		const uint8_t code = *str++;
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
		_paletteChanged = true;
	}
	return 0xFFFF;
}

void Game::loadLevelMap() {
	debug(DBG_GAME, "Game::loadLevelMap() room=%d", _currentRoom);
	DecodeBuffer buf;
	initDecodeBuffer(&buf, 0, 0, false, true, _frontLayer, 0);
	_res.loadLevelRoom(_currentLevel, _currentRoom, &buf);
	memcpy(_backLayer, _frontLayer, kScreenWidth * kScreenHeight);
	_res.setupLevelClut(_currentLevel, _palette);
	_paletteChanged = true;
	_backgroundChanged = true;
}

void Game::loadLevelData() {
	_cutId = _cutsceneLevels[_currentLevel];

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
	const uint8_t *dataPtr = _res.getImageData(_res._icn, iconNum);
	addImageToList(buf.x, buf.y, READ_BE_UINT16(dataPtr), READ_BE_UINT16(dataPtr + 2), false, true, _res._icn, iconNum);
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

void Game::initInventory() {
	if (!_inventoryOn) {
		_inventoryCurrentItem = 0;
		_inventoryItemsCount = 0;
		int i = _pgeLive[0].current_inventory_PGE;
		while (i != 0xFF) {
			_inventoryItems[_inventoryItemsCount].icon_num = _res._pgeInit[i].icon_num;
			_inventoryItems[_inventoryItemsCount].init_pge = &_res._pgeInit[i];
			_inventoryItems[_inventoryItemsCount].live_pge = &_pgeLive[i];
			if (_res._pgeInit[i].object_id != kGunObject) {
				++_inventoryItemsCount;
			}
			i = _pgeLive[i].next_inventory_PGE;
		}
		_inventoryItems[_inventoryItemsCount].icon_num = 0xFF;
	} else {
		pge_setCurrentInventoryObject(_inventoryItems[_inventoryCurrentItem].live_pge);
	}
	_inventoryOn = !_inventoryOn;
}

void Game::doInventory() {
	addHotspot(Hotspot::kIdUseInventory, 232, 8, 16, 16);
	LivePGE *pge = &_pgeLive[0];
	if (pge->life > 0 && pge->current_inventory_PGE != 0xFF) {
//		playSound(66, 0);

		drawIcon(31, 56, 140, 0xF);
		const int h = (_inventoryItemsCount - 1) / 4 + 1;
		const int y = _inventoryCurrentItem / 4;

		for (int i = 0; i < 4; ++i) {
			int currentItem = y * 4 + i;
			if (_inventoryItems[currentItem].icon_num == 0xFF) {
				break;
			}
			const int xPos = 72 + i * 32;
			drawIcon(_inventoryItems[currentItem].icon_num, xPos, 157, 0xA);
			addHotspot(Hotspot::kIdSelectInventoryObject, xPos, 157, 16, 16);
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
			addHotspot(Hotspot::kIdScrollDownInventory, 120, 176, 16, 16);
		}
		if (y != h - 1) {
			drawIcon(33, 120, 143, 0xA); // up arrow
			addHotspot(Hotspot::kIdScrollUpInventory, 120, 143, 16, 16);
		}
		if (_pi.dirMask & PlayerInput::kDirectionUp) {
			_pi.dirMask &= ~PlayerInput::kDirectionUp;
			if (y < h - 1) {
				_inventoryCurrentItem = (y + 1) * 4;
			}
		}
		if (_pi.dirMask & PlayerInput::kDirectionDown) {
			_pi.dirMask &= ~PlayerInput::kDirectionDown;
			if (y > 0) {
				_inventoryCurrentItem = (y - 1) * 4;
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
			if (_inventoryCurrentItem < _inventoryItemsCount - 1) {
				const int num = _inventoryCurrentItem % 4;
				if (num < 3) {
					++_inventoryCurrentItem;
				}
			}
		}
//		playSound(66, 0);
	}
}

void AnimBuffers::addState(uint8 stateNum, int16 x, int16 y, LivePGE *pge) {
	debug(DBG_GAME, "AnimBuffers::addState() stateNum=%d x=%d y=%d pge=0x%X", stateNum, x, y, pge);
	assert(stateNum < 4);
	AnimBufferState *state = _states[stateNum];
	state->x = x;
	state->y = y;
	state->pge = pge;
	++_curPos[stateNum];
	++_states[stateNum];
}

void Game::clearHotspots() {
	_hotspotsCount = 0;
}

void Game::addHotspot(int id, int x, int y, int w, int h) {
	assert(_hotspotsCount < ARRAYSIZE(_hotspotsList));
	Hotspot *hs = &_hotspotsList[_hotspotsCount];
	hs->id = id;
	hs->x = x * kHotspotCoordScale;
	hs->y = y * kHotspotCoordScale;
	hs->x2 = (x + w) * kHotspotCoordScale;
	hs->y2 = (y + h) * kHotspotCoordScale;
	++_hotspotsCount;
}

void Game::clearImagesList() {
	_imagesCount = 0;
}

void Game::addImageToList(int x, int y, int w, int h, bool xflip, bool erase, uint8_t *dataPtr, int num) {
	assert(_imagesCount < ARRAYSIZE(_imagesList));
	Image *i = &_imagesList[_imagesCount];
	i->x = x;
	i->y = y;
	i->w = w;
	i->h = h;
	i->xflip = xflip;
	i->yflip = false;
	i->erase = erase;
	i->dataPtr = dataPtr;
	i->num = num;
	++_imagesCount;
}

