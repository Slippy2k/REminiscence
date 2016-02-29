
#include <time.h>
#include "game.h"
#include "util.h"


Game::Game(ResourceData &res, const char *savePath)
	: _res(res), _saveDirectory(savePath) {
	_mix = 0;
	_randSeed = time(0);
	_skillLevel = 1;
	_currentLevel = 0;
	_frontLayer = 0;
	_inventoryOn = false;
	_hotspotsCount = 0;
	_shakeOffset = 0;
	_res.setupTextClut(_palette);
	memset(&_pi, 0, sizeof(_pi));
}

Game::~Game() {
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
	_textColor = 0xE8;
	_nextTextSegment = 0;
	_voiceSound = 0;
	_gameOver = false;
	memset(_sounds, 0, sizeof(_sounds));
}

void Game::initGame() {
//	_vid._unkPalSlot1 = 0;
//	_vid._unkPalSlot2 = 0;
	_score = 0;
	loadLevelData();
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
					_pi.enter = (_pi.touch.press == PlayerInput::kTouchDown);
					break;
				case Hotspot::kIdSelectInventoryObject:
					if (_pi.touch.press == PlayerInput::kTouchDown) {
						const int num = (_pi.touch.x / kHotspotCoordScale - 64) / 32;
						if (num >= 0 && num <= 3) {
							const int current = (_inventoryCurrentItem & ~3) | num;
							if (current < _inventoryItemsCount) {
								_inventoryCurrentItem = current;
							}
						}
					}
					break;
				case Hotspot::kIdScrollDownInventory:
					if (_pi.touch.press == PlayerInput::kTouchUp) {
						const int current = (_inventoryCurrentItem - 4) & ~3;
						if (current >= 0) {
							_inventoryCurrentItem = current;
						}
					}
					break;
				case Hotspot::kIdScrollUpInventory:
					if (_pi.touch.press == PlayerInput::kTouchUp) {
						const int current = (_inventoryCurrentItem + 4) & ~3;
						if (current < _inventoryItemsCount) {
							_inventoryCurrentItem = current;
						}
					}
					break;
				case Hotspot::kIdSelectLevel:
					if (_pi.touch.press == PlayerInput::kTouchDown) {
                                                const int num = (_pi.touch.y / kHotspotCoordScale - 24) / 16;
						if (num >= 0 && num < 7) {
							_currentLevel = num;
						}
					} else if (_pi.touch.press == PlayerInput::kTouchUp) {
						_pi.enter = true;
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
				_gameOver = true;
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
//		memcpy(_frontLayer, _backLayer, kScreenWidth * kScreenHeight);
		pge_getInput();
		pge_prepare();
		col_prepareRoomState();
		uint8_t oldLevel = _currentLevel;
		for (uint16_t i = 0; i < _res._pgeNum; ++i) {
			LivePGE *pge = _pge_liveTable2[i];
			if (pge) {
				_col_currentPiegeGridPosY = (pge->pos_y / 36) & ~1;
				_col_currentPiegeGridPosX = (pge->pos_x + 8) >> 4;
				pge_process(pge);
			}
		}
		if (oldLevel != _currentLevel) {
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
					drawIcon(32, 4 + hs->x / kHotspotCoordScale, 4 + hs->y / kHotspotCoordScale);
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
		_mix->stopAll();
#if 0
		_cut.play();
#endif
	}
}

void Game::drawCurrentInventoryItem() {
	int src = _pgeLive[0].current_inventory_PGE;
	if (src != 0xFF) {
		drawIcon(_res._pgeInit[src].icon_num, 232, 8);
		addHotspot(Hotspot::kIdUseInventory, 232 - 4, 8 - 4, 24, 24);
		while (src != 0xFF) {
			if (_res._pgeInit[src].object_id == kGunObject) {
				drawIcon(_res._pgeInit[src].icon_num, 208, 8);
				addHotspot(Hotspot::kIdUseGun, 208 - 4, 8 - 4, 24, 24);
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
	int8_t obj = col_findCurrentCollidingObject(pge, 3, 0xFF, 0xFF, &pge);
	if (obj == 0) {
		obj = col_findCurrentCollidingObject(pge, 0xFF, 5, 9, &pge);
	}
	if (obj > 0) {
		_printLevelCodeCounter = 0;
		if (_textToDisplay == 0xFFFF) {
			const uint8_t icon_num = obj - 1;
			drawIcon(icon_num, 80, 8);
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
	const uint8_t *str = _res.getStringData(ResourceData::kGameString + _textToDisplay);
	const int segmentsCount = *str++;
	bool nextText = false;
	if (_voiceSound != 0 && !_mix->isPlayingSound(_voiceSound)) {
		nextText = true;
	}
	if (_pi.backspace || _pi.shift) {
		_pi.backspace = false;
		_pi.shift = false;
		nextText = true;
		if (_voiceSound != 0) {
			_mix->stopSound(_voiceSound);
			_voiceSound = 0;
		}
	}
	if (nextText) {
		// change to next text segment
		if (_nextTextSegment >= segmentsCount) {
			_nextTextSegment = 0;
			_textToDisplay = 0xFFFF;
			_textColor = 0xE8;
			_voiceSound = 0;
			return;
                }
	} else if (_nextTextSegment > 0) {
		return;
	}
	uint8_t *wav = _res.getVoiceSegment(_textToDisplay, _nextTextSegment);
	if (wav) {
		_voiceSound = _mix->playSoundWav(wav);
		free(wav);
	}
	_gfxTextsCount = 0;
		// goto to current segment
		for (int i = 0; i < _nextTextSegment; ++i) {
			const int segmentLength = *str++;
			str += segmentLength;
		}
		int len = *str++;
		if (*str == '@') {
			++str;
			switch (*str) {
			case '1':
				_textColor = 0xE9;
				break;
			case '2':
				_textColor = 0xEB;
				break;
			}
			++str;
			len -= 2;
		}
		drawIcon(_currentInventoryIconNum, 80, 8);
		int lineOffset = 26;
		while (len > 0) {
			const uint8_t *next = (const uint8_t *)memchr(str, 0x7C, len);
			if (!next) {
				drawString(str, len, (176 - len * 8) / 2, lineOffset, _textColor);
				break;
			}
			const int lineLength = next - str;
			drawString(str, lineLength, (176 - lineLength * 8) / 2, lineOffset, _textColor);
			str = next + 1;
			len -= lineLength + 1;
			lineOffset += 8;
		}
	++_nextTextSegment;
}

void Game::prepareAnims() {
	if (!(_currentRoom & 0x80) && _currentRoom < 0x40) {
		int8_t pge_room;
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

void Game::prepareAnimsHelper(LivePGE *pge, int16_t dx, int16_t dy) {
	debug(DBG_GAME, "Game::prepareAnimsHelper() dx=0x%X dy=0x%X pge_num=%d pge->flags=0x%X pge->anim_number=0x%X", dx, dy, pge - &_pgeLive[0], pge->flags, pge->anim_number);
	int16_t xpos, ypos;
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

static void initDecodeBuffer(DecodeBuffer *buf, int x, int y, const uint8_t *dataPtr, bool xflip) {
	memset(buf, 0, sizeof(DecodeBuffer));
	buf->x = x * 2;
	buf->y = y * 2;
	if (dataPtr) {
		if (xflip) {
			buf->x += (int16_t)READ_BE_UINT16(dataPtr + 4) - READ_BE_UINT16(dataPtr) - 1;
		} else {
			buf->x -= (int16_t)READ_BE_UINT16(dataPtr + 4);
		}
		buf->y -= (int16_t)READ_BE_UINT16(dataPtr + 6);
	}
}

void Game::drawPiege(LivePGE *pge, int x, int y) {
	const bool xflip = (pge->flags & 2);
	DecodeBuffer buf;
	if (pge->flags & 8) {
		const uint8_t *dataPtr = _res.getImageData(_res._spc, pge->anim_number);
		if (!dataPtr) return;
		initDecodeBuffer(&buf, x, y, dataPtr, xflip);
		addImageToGfxList(buf.x, buf.y, READ_BE_UINT16(dataPtr), READ_BE_UINT16(dataPtr + 2), xflip, _eraseBackground, _res._spc, pge->anim_number);
	} else {
		if (pge->index == 0) {
			if (pge->anim_number == 0x386) return;
			const int frame = _res.getPersoFrame(pge->anim_number);
			const uint8_t *dataPtr = _res.getImageData(_res._perso, frame);
			if (!dataPtr) return;
			initDecodeBuffer(&buf, x, y, dataPtr, xflip);
			addImageToGfxList(buf.x, buf.y, READ_BE_UINT16(dataPtr), READ_BE_UINT16(dataPtr + 2), xflip, _eraseBackground, _res._perso, frame);
		} else {
			const int frame = _res.getMonsterFrame(pge->anim_number);
			const uint8_t *dataPtr = _res.getImageData(_res._monster, frame);
			if (!dataPtr) return;
			initDecodeBuffer(&buf, x, y, dataPtr, xflip);
			addImageToGfxList(buf.x, buf.y, READ_BE_UINT16(dataPtr), READ_BE_UINT16(dataPtr + 2), xflip, _eraseBackground, _res._monster, frame);
		}
	}
}

void Game::drawString(const unsigned char *str, int len, int x, int y, int color) {
	addTextToGfxList(x, y, len, str, color);
}

void Game::drawAnimBuffer(uint8_t stateNum, AnimBufferState *state) {
	debug(DBG_GAME, "Game::drawAnimBuffer() state=%d", stateNum);
	assert(stateNum < 4);
	_animBuffers._states[stateNum] = state;
	const uint8_t lastPos = _animBuffers._curPos[stateNum];
	if (lastPos != 0xFF) {
		uint8_t numAnims = lastPos + 1;
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

	const uint8_t *mList = _monsterListLevels[_currentLevel];
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
//	_res.loadLevelRoom(_currentLevel, _currentRoom, &buf);
//	memcpy(_backLayer, _frontLayer, kScreenWidth * kScreenHeight);
	_res.setupRoomClut(_currentLevel, _currentRoom, _palette);
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
	uint16_t n = _res._pgeNum;
	while (n--) {
		pge_loadForCurrentLevel(n);
	}
	for (uint16_t i = 0; i < _res._pgeNum; ++i) {
		if (_res._pgeInit[i].skill <= _skillLevel) {
			LivePGE *pge = &_pgeLive[i];
			pge->next_PGE_in_room = _pge_liveTable1[pge->room_location];
			_pge_liveTable1[pge->room_location] = pge;
		}
	}
	pge_resetGroups();
}

void Game::drawIcon(uint8_t iconNum, int x, int y) {
	DecodeBuffer buf;
	initDecodeBuffer(&buf, x, y, 0, false);
	const uint8_t *dataPtr = _res.getImageData(_res._icn, iconNum);
	addImageToGfxList(buf.x, buf.y, READ_BE_UINT16(dataPtr), READ_BE_UINT16(dataPtr + 2), false, true, _res._icn, iconNum);
}

void Game::playSound(uint8_t num, uint8_t softVol) {
	if (num < 66) {
		static const int8_t table[] = {
			 0, -1,  1,  2,  3,  4, -1,  5,  6,  7,  8,  9, 10, 11, -1, 12,
			13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, 26, 27,
			28, -1, 29, -1, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
			42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, -1, 53, 54, 55, 56,
			-1, 57
		};
		if (table[num] == -1) {
			return;
		}
		if (_sounds[num] != 0) {
			// ignore already playing
			if (_mix->isPlayingSound(_sounds[num])) {
				return;
			}
			_sounds[num] = 0;
		}
		int freq = 8000;
		uint32_t size;
		const uint8_t *data = _res.getSoundData(table[num], &freq, &size);
		if (data) {
			const int volume = 255 >> softVol;
			if (memcmp(data, "RIFF", 4) == 0) {
				_sounds[num] = _mix->playSoundWav(data, volume);
			} else {
				_sounds[num] = _mix->playSoundRaw(data, size, freq, volume);
			}
		}
	} else if (num < 76) {
		uint8_t *wav = _res.getSfxData(num);
		if (wav) {
			_mix->playSoundWav(wav);
			free(wav);
		}
	}
}

uint16_t Game::getRandomNumber() {
	uint32_t n = _randSeed * 2;
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

		drawIcon(31, 56, 140);
		const int h = (_inventoryItemsCount - 1) / 4 + 1;
		const int y = _inventoryCurrentItem / 4;

		addHotspot(Hotspot::kIdSelectInventoryObject, 72 - 8, 157, 32 * 4, 16);
		for (int i = 0; i < 4; ++i) {
			int currentItem = y * 4 + i;
			if (_inventoryItems[currentItem].icon_num == 0xFF) {
				break;
			}
			const int xPos = 72 + i * 32;
			drawIcon(_inventoryItems[currentItem].icon_num, xPos, 157);
			if (_inventoryCurrentItem == currentItem) {
				drawIcon(32, xPos, 157);
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
			drawIcon(34, 120, 176); // down arrow
			addHotspot(Hotspot::kIdScrollDownInventory, 120, 176, 16, 16);
		}
		if (y != h - 1) {
			drawIcon(33, 120, 143); // up arrow
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

void AnimBuffers::addState(uint8_t stateNum, int16_t x, int16_t y, LivePGE *pge) {
	debug(DBG_GAME, "AnimBuffers::addState() stateNum=%d x=%d y=%d pge=0x%X", stateNum, x, y, pge);
	assert(stateNum < 4);
	AnimBufferState *state = _states[stateNum];
	state->x = x;
	state->y = y;
	state->pge = pge;
	++_curPos[stateNum];
	++_states[stateNum];
}

void Game::doTitle() {
	static const uint8_t selectedColor = 0xE4;
	static const uint8_t defaultColor = 0xE8;
	for (int i = 0; i < 7; ++i) {
		const char *str = _levelNames[i];
		const int len = strlen(str);
		drawString((const uint8_t *)str, len, 24, 24 + i * 16, (_currentLevel == i) ? selectedColor : defaultColor);
	}
	addHotspot(Hotspot::kIdSelectLevel, 24, 24, 256 - 24 * 2, 16 * 7);
	if (_pi.dirMask & PlayerInput::kDirectionUp) {
		_pi.dirMask &= ~PlayerInput::kDirectionUp;
		if (_currentLevel > 0) {
			--_currentLevel;
		}
	}
	if (_pi.dirMask & PlayerInput::kDirectionDown) {
		_pi.dirMask &= ~PlayerInput::kDirectionDown;
		if (_currentLevel < 6) {
			++_currentLevel;
		}
	}
}

void Game::clearHotspots() {
	if (_pi.touch.press == PlayerInput::kTouchUp) {
		_pi.touch.press = PlayerInput::kTouchNone;
	}
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

void Game::addImageToGfxList(int x, int y, int w, int h, bool xflip, bool erase, uint8_t *dataPtr, int num) {
	assert(_gfxImagesCount < ARRAYSIZE(_gfxImagesList));
	GfxImage *i = &_gfxImagesList[_gfxImagesCount];
	i->x = x;
	i->y = y;
	i->w = w;
	i->h = h;
	i->xflip = xflip;
	i->yflip = false;
	i->erase = erase;
	i->dataPtr = dataPtr;
	i->num = num;
	++_gfxImagesCount;
}

void Game::clearGfxList() {
	_gfxImagesCount = 0;
	_gfxTextsCount = 0;
}

void Game::saveGfxList() {
}

void Game::addTextToGfxList(int x, int y, int len, const uint8_t *dataPtr, uint8_t color) {
	assert(_gfxTextsCount < ARRAYSIZE(_gfxTextsList));
	GfxText *gt = &_gfxTextsList[_gfxTextsCount];
	gt->x = x * 2;
	gt->y = y * 2;
	if (len > sizeof(gt->data)) {
		len = sizeof(gt->data);
	}
	gt->len = len;
	memcpy(gt->data, dataPtr, len);
	gt->color = color;
	++_gfxTextsCount;
}

