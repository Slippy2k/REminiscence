
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

	while (!_stub->_pi.quit && _menu.handleTitleScreen(_skillLevel, _currentLevel)) {
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
//	_cut._deathCutsceneId = 0xFFFF;
	_pge_opTempVar2 = 0xFFFF;
	_deathCutsceneCounter = 0;
	_saveStateCompleted = false;
	_loadMap = true;
	pge_resetGroups();
	_blinkingConradCounter = 0;
	_pge_processOBJ = false;
	_pge_opTempVar1 = 0;
	_textToDisplay = 0xFFFF;
}

void Game::initGame() {
//	_vid._unkPalSlot1 = 0;
//	_vid._unkPalSlot2 = 0;
	_score = 0;
	_firstBankData = _bankData;
	_lastBankData = _bankData + sizeof(_bankData);
	loadLevelData();
	resetGameState();
}

void Game::doTick() {
#if 0
		playCutscene();
		if (_cut._id == 0x3D) {
			showFinalScore();
			break;
		}
#endif
		if (_deathCutsceneCounter) {
			--_deathCutsceneCounter;
			if (_deathCutsceneCounter == 0) {
#if 0
				playCutscene(_cut._deathCutsceneId);
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
			changeLevel();
			_pge_opTempVar1 = 0;
			return;
		}
		if (_loadMap) {
			if (_currentRoom == 0xFF) {
//				_cut._id = 6;
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
#if 0
		_vid.updateScreen();
		updateTiming();
		drawStoryTexts();
		if (_stub->_pi.backspace) {
			_stub->_pi.backspace = false;
			handleInventory();
		}
		if (_stub->_pi.escape) {
			_stub->_pi.escape = false;
			if (handleConfigPanel()) {
				break;
			}
		}
		inp_handleSpecialKeys();
#endif
}

#if 0
void Game::updateTiming() {
	static uint32 tstamp = 0;
	int32 delay = _stub->getTimeStamp() - tstamp;
	int32 pause = (_stub->_pi.dbgMask & PlayerInput::DF_FASTMODE) ? 20 : 30;
	pause -= delay;
	if (pause > 0) {
		_stub->sleep(pause);
	}
	tstamp = _stub->getTimeStamp();
}
#endif

void Game::playCutscene(int id) {
#if 0
	if (id != -1) {
		_cut._id = id;
	}
	if (_cut._id != 0xFFFF) {
		_sfxPly.stop();
		_cut.play();
	}
#endif
}

void Game::inp_handleSpecialKeys() {
#if 0
	if (_stub->_pi.dbgMask & PlayerInput::DF_SETLIFE) {
		_pgeLive[0].life = 0x7FFF;
	}
	if (_stub->_pi.load) {
		loadGameState(_stateSlot);
		_stub->_pi.load = false;
	}
	if (_stub->_pi.save) {
		saveGameState(_stateSlot);
		_stub->_pi.save = false;
	}
	if (_stub->_pi.stateSlot != 0) {
		int8 slot = _stateSlot + _stub->_pi.stateSlot;
		if (slot >= 1 && slot < 100) {
			_stateSlot = slot;
			debug(DBG_INFO, "Current game state slot is %d", _stateSlot);
		}
		_stub->_pi.stateSlot = 0;
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

void Game::showFinalScore() {
#if 0
	playCutscene(0x49);
	char textBuf[50];
	sprintf(textBuf, "SCORE %08u", _score);
	_vid.drawString(textBuf, (256 - strlen(textBuf) * 8) / 2, 40, 0xE5);
	strcpy(textBuf, _menu._passwords[7][_skillLevel]);
	_vid.drawString(textBuf, (256 - strlen(textBuf) * 8) / 2, 16, 0xE7);
	while (!_stub->_pi.quit) {
		_stub->copyRect(0, 0, kScreenWidth, kScreenHeight, _vid._frontLayer, 256);
		_stub->updateScreen(0);
		_stub->processEvents();
		if (_stub->_pi.enter) {
			_stub->_pi.enter = false;
			break;
		}
		_stub->sleep(100);
	}
#endif
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
	while (!_stub->_pi.quit) {
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
		if (_stub->_pi.dirMask & PlayerInput::DIR_UP) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_UP;
			current = (current + 3) % 4;
		}
		if (_stub->_pi.dirMask & PlayerInput::DIR_DOWN) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_DOWN;
			current = (current + 1) % 4;
		}
		if (_stub->_pi.dirMask & PlayerInput::DIR_LEFT) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_LEFT;
			--_stateSlot;
			if (_stateSlot < 1) {
				_stateSlot = 1;
			}
		}
		if (_stub->_pi.dirMask & PlayerInput::DIR_RIGHT) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_RIGHT;
			++_stateSlot;
			if (_stateSlot > 99) {
				_stateSlot = 99;
			}
		}
		if (prev != current) {
			SWAP(colors[prev], colors[current]);
		}
		if (_stub->_pi.enter) {
			_stub->_pi.enter = false;
			switch (current) {
			case MENU_ITEM_LOAD:
				_stub->_pi.load = true;
				break;
			case MENU_ITEM_SAVE:
				_stub->_pi.save = true;
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
	while (timeout >= 0 && !_stub->_pi.quit) {
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
		if (_stub->_pi.dirMask & PlayerInput::DIR_UP) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_UP;
			if (current_color > 0) {
				SWAP(colors[current_color], colors[current_color - 1]);
				--current_color;
			}
		}
		if (_stub->_pi.dirMask & PlayerInput::DIR_DOWN) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_DOWN;
			if (current_color < 1) {
				SWAP(colors[current_color], colors[current_color + 1]);
				++current_color;
			}
		}
		if (_stub->_pi.enter) {
			_stub->_pi.enter = false;
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
#if 0
	if (_printLevelCodeCounter != 0) {
		--_printLevelCodeCounter;
		if (_printLevelCodeCounter != 0) {
			char levelCode[50];
			sprintf(levelCode, "CODE: %s", _menu._passwords[_currentLevel][_skillLevel]);
			_vid.drawString(levelCode, (kScreenWidth - strlen(levelCode) * 8) / 2, 16, 0xE7);
		}
	}
#endif
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
#if 0
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
			uint8 txt_num = pge->init_PGE->text_num;
			const char *str = (const char *)_res._tbn + READ_BE_UINT16(_res._tbn + txt_num * 2);
			_vid.drawString(str, (176 - strlen(str) * 8) / 2, 26, 0xE6);
			if (icon_num == 2) {
				printSaveStateCompleted();
				return;
			}
		} else {
			_currentInventoryIconNum = obj - 1;
		}
	}
	_saveStateCompleted = false;
#endif
}

void Game::drawStoryTexts() {
#if 0
	if (_textToDisplay != 0xFFFF) {
		uint16 text_col_mask = 0xE8;
		const uint8 *str = _res.getGameString(_textToDisplay);
		memcpy(_vid._tempLayer, _vid._frontLayer, kScreenWidth * kScreenHeight);
		int textSpeechSegment = 0;
		while (!_stub->_pi.quit) {
			drawIcon(_currentInventoryIconNum, 80, 8, 0xA);
			if (*str == 0xFF) {
				text_col_mask = READ_BE_UINT16(str + 1);
				str += 3;
			}
			int16 text_y_pos = 26;
			while (1) {
				uint16 len = getLineLength(str);
				str = (const uint8 *)_vid.drawString((const char *)str, (176 - len * 8) / 2, text_y_pos, text_col_mask);
				text_y_pos += 8;
				if (*str == 0 || *str == 0xB) {
					break;
				}
				++str;
			}
			MixerChunk chunk;
			_res.load_VCE(_textToDisplay, textSpeechSegment++, &chunk.data, &chunk.len);
			if (chunk.data) {
				_mix.play(&chunk, 32000, Mixer::MAX_VOLUME);
			}
			_vid.updateScreen();
			while (!_stub->_pi.backspace && !_stub->_pi.quit) {
				inp_update();
				_stub->sleep(80);
			}
			if (chunk.data) {
				_mix.stopAll();
				free(chunk.data);
			}
			_stub->_pi.backspace = false;
			if (*str == 0) {
				break;
			}
			++str;
			memcpy(_vid._frontLayer, _vid._tempLayer, kScreenWidth * kScreenHeight);
		}
		_textToDisplay = 0xFFFF;
	}
#endif
}

void Game::prepareAnims() {
	if (!(_currentRoom & 0x80) && _currentRoom < 0x40) {
		int8 pge_room;
		LivePGE *pge = _pge_liveTable1[_currentRoom];
		while (pge) {
			prepareAnimsHelper(pge, 0, 0);
			pge = pge->next_PGE_in_room;
		}
		pge_room = _res._ctData[CT_UP_ROOM + _currentRoom];
		if (pge_room >= 0 && pge_room < 0x40) {
			pge = _pge_liveTable1[pge_room];
			while (pge) {
				if ((pge->init_PGE->object_type != 10 && pge->pos_y > 176) || (pge->init_PGE->object_type == 10 && pge->pos_y > 216)) {
					prepareAnimsHelper(pge, 0, -216);
				}
				pge = pge->next_PGE_in_room;
			}
		}
		pge_room = _res._ctData[CT_DOWN_ROOM + _currentRoom];
		if (pge_room >= 0 && pge_room < 0x40) {
			pge = _pge_liveTable1[pge_room];
			while (pge) {
				if (pge->pos_y < 48) {
					prepareAnimsHelper(pge, 0, 216);
				}
				pge = pge->next_PGE_in_room;
			}
		}
		pge_room = _res._ctData[CT_LEFT_ROOM + _currentRoom];
		if (pge_room >= 0 && pge_room < 0x40) {
			pge = _pge_liveTable1[pge_room];
			while (pge) {
				if (pge->pos_x > 224) {
					prepareAnimsHelper(pge, -256, 0);
				}
				pge = pge->next_PGE_in_room;
			}
		}
		pge_room = _res._ctData[CT_RIGHT_ROOM + _currentRoom];
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
//		const uint8 *dataPtr = _res._spr_off[pge->anim_number];
// TODO: _perso
		const uint8 *dataPtr = _res.getImageData(_res._perso, _res.getSpriteFrame(pge->anim_number));
		if (dataPtr == 0) {
			return;
		}

		if (pge->flags & 2) {
			xpos = (int8)dataPtr[0] + dx + pge->pos_x;
			uint8 _cl = dataPtr[2];
			if (_cl & 0x40) {
				_cl = dataPtr[3];
			} else {
				_cl &= 0x3F;
			}
			xpos -= _cl;
		} else {
			xpos = dx + pge->pos_x - (int8)dataPtr[0];
		}

		ypos = dy + pge->pos_y - (int8)dataPtr[1] + 2;
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
#if 0
		assert(pge->anim_number < _res._numSpc);
		const uint8 *dataPtr = _res._spc + READ_BE_UINT16(_res._spc + pge->anim_number * 2);
		xpos = dx + pge->pos_x + 8;
		ypos = dy + pge->pos_y + 2;

		if (pge->init_PGE->object_type == 11) {
			_animBuffers.addState(3, xpos, ypos, dataPtr, pge);
		} else if (pge->flags & 0x10) {
			_animBuffers.addState(2, xpos, ypos, dataPtr, pge);
		} else {
			_animBuffers.addState(0, xpos, ypos, dataPtr, pge);
		}
#endif
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

void Game::drawAnimBuffer(uint8 stateNum, AnimBufferState *state) {
	debug(DBG_GAME, "Game::drawAnimBuffer() state=%d", stateNum);
	assert(stateNum < 4);
	_animBuffers._states[stateNum] = state;
	uint8 lastPos = _animBuffers._curPos[stateNum];
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
#if 0
				if (!(state->dataPtr[-2] & 0x80)) {
					decodeCharacterFrame(state->dataPtr, _res._memBuf);
					drawCharacter(_res._memBuf, state->x, state->y, state->dataPtr[-1], state->dataPtr[-2], pge->flags);
				} else {
					drawCharacter(state->dataPtr, state->x, state->y, state->dataPtr[-1], state->dataPtr[-2], pge->flags);
				}
#else
drawCharacter(state->dataPtr, state->x, state->y, state->dataPtr[-1], state->dataPtr[-2], pge->flags);
#endif
			} else {
				drawObject(state->dataPtr, state->x, state->y, pge->flags);
			}
			--state;
		} while (--numAnims != 0);
	}
}

void Game::drawObject(const uint8 *dataPtr, int16 x, int16 y, uint8 flags) {
#if 0
	debug(DBG_GAME, "Game::drawObject() dataPtr[]=0x%X dx=%d dy=%d",  dataPtr[0], (int8)dataPtr[1], (int8)dataPtr[2]);
	assert(dataPtr[0] < 0x4A);
	uint8 slot = _res._rp[dataPtr[0]];
	uint8 *data = findBankData(slot);
	if (data == 0) {
		data = loadBankData(slot);
	}
	_bankDataPtrs = data;
	int16 posy = y - (int8)dataPtr[2];
	int16 posx = x;
	if (flags & 2) {
		posx += (int8)dataPtr[1];
	} else {
		posx -= (int8)dataPtr[1];
	}
	int i = dataPtr[5];
	dataPtr += 6;
	while (i--) {
		drawObjectFrame(dataPtr, posx, posy, flags);
		dataPtr += 4;
	}
#endif
}

void Game::drawObjectFrame(const uint8 *dataPtr, int16 x, int16 y, uint8 flags) {
#if 0
	debug(DBG_GAME, "Game::drawObjectFrame(0x%X, %d, %d, 0x%X)", dataPtr, x, y, flags);
	const uint8 *src = _bankDataPtrs + dataPtr[0] * 32;

	int16 sprite_y = y + dataPtr[2];
	int16 sprite_x;
	if (flags & 2) {
		sprite_x = x - dataPtr[1] - (((dataPtr[3] & 0xC) + 4) * 2);
	} else {
		sprite_x = x + dataPtr[1];
	}

	uint8 sprite_flags = dataPtr[3];
	if (flags & 2) {
		sprite_flags ^= 0x10;
	}

	uint8 sprite_h = (((sprite_flags >> 0) & 3) + 1) * 8;
	uint8 sprite_w = (((sprite_flags >> 2) & 3) + 1) * 8;

	int size = sprite_w * sprite_h / 2;
	for (int i = 0; i < size; ++i) {
		uint8 col = *src++;
		_res._memBuf[i * 2 + 0] = (col & 0xF0) >> 4;
		_res._memBuf[i * 2 + 1] = (col & 0x0F) >> 0;
	}

	src = _res._memBuf;
	bool sprite_mirror_x = false;
	int16 sprite_clipped_w;
	if (sprite_x >= 0) {
		sprite_clipped_w = sprite_x + sprite_w;
		if (sprite_clipped_w < 256) {
			sprite_clipped_w = sprite_w;
		} else {
			sprite_clipped_w = 256 - sprite_x;
			if (sprite_flags & 0x10) {
				sprite_mirror_x = true;
				src += sprite_w - 1;
			}
		}
	} else {
		sprite_clipped_w = sprite_x + sprite_w;
		if (!(sprite_flags & 0x10)) {
			src -= sprite_x;
			sprite_x = 0;
		} else {
			sprite_mirror_x = true;
			src += sprite_x + sprite_w - 1;
			sprite_x = 0;
		}
	}
	if (sprite_clipped_w <= 0) {
		return;
	}

	int16 sprite_clipped_h;
	if (sprite_y >= 0) {
		sprite_clipped_h = 224 - sprite_h;
		if (sprite_y < sprite_clipped_h) {
			sprite_clipped_h = sprite_h;
		} else {
			sprite_clipped_h = 224 - sprite_y;
		}
	} else {
		sprite_clipped_h = sprite_h + sprite_y;
		src -= sprite_w * sprite_y;
		sprite_y = 0;
	}
	if (sprite_clipped_h <= 0) {
		return;
	}

	if (!sprite_mirror_x && (sprite_flags & 0x10)) {
		src += sprite_w - 1;
	}

	uint32 dst_offset = 256 * sprite_y + sprite_x;
	uint8 sprite_col_mask = (flags & 0x60) >> 1;

	if (_eraseBackground) {
		if (!(sprite_flags & 0x10)) {
			_vid.drawSpriteSub1(src, _vid._frontLayer + dst_offset, sprite_w, sprite_clipped_h, sprite_clipped_w, sprite_col_mask);
		} else {
			_vid.drawSpriteSub2(src, _vid._frontLayer + dst_offset, sprite_w, sprite_clipped_h, sprite_clipped_w, sprite_col_mask);
		}
	} else {
		if (!(sprite_flags & 0x10)) {
			_vid.drawSpriteSub3(src, _vid._frontLayer + dst_offset, sprite_w, sprite_clipped_h, sprite_clipped_w, sprite_col_mask);
		} else {
			_vid.drawSpriteSub4(src, _vid._frontLayer + dst_offset, sprite_w, sprite_clipped_h, sprite_clipped_w, sprite_col_mask);
		}
	}
	_vid.markBlockAsDirty(sprite_x, sprite_y, sprite_clipped_w, sprite_clipped_h);
#endif
}

void Game::decodeCharacterFrame(const uint8 *dataPtr, uint8 *dstPtr) {
	int n = READ_BE_UINT16(dataPtr); dataPtr += 2;
	uint16 len = n * 2;
	uint8 *dst = dstPtr + 0x400;
	while (n--) {
		uint8 c = *dataPtr++;
		dst[0] = (c & 0xF0) >> 4;
		dst[1] = (c & 0x0F) >> 0;
		dst += 2;
	}
	dst = dstPtr;
	const uint8 *src = dstPtr + 0x400;
	do {
		uint8 c1 = *src++;
		if (c1 == 0xF) {
			uint8 c2 = *src++;
			uint16 c3 = *src++;
			if (c2 == 0xF) {
				c1 = *src++;
				c2 = *src++;
				c3 = (c3 << 4) | c1;
				len -= 2;
			}
			memset(dst, c2, c3 + 4);
			dst += c3 + 4;
			len -= 3;
		} else {
			*dst++ = c1;
			--len;
		}
	} while (len != 0);
}


void Game::drawCharacter(const uint8 *dataPtr, int16 pos_x, int16 pos_y, uint8 a, uint8 b, uint8 flags) {
	debug(DBG_GAME, "Game::drawCharacter(0x%X, %d, %d, 0x%X, 0x%X, 0x%X)", dataPtr, pos_x, pos_y, a, b, flags);

	bool var16 = false; // sprite_mirror_y
	if (b & 0x40) {
		b &= 0xBF;
		SWAP(a, b);
		var16 = true;
	}
	uint16 sprite_h = a;
	uint16 sprite_w = b;

	const uint8 *src = dataPtr;
	bool var14 = false;

	int16 sprite_clipped_w;
	if (pos_x >= 0) {
		if (pos_x + sprite_w < 256) {
			sprite_clipped_w = sprite_w;
		} else {
			sprite_clipped_w = 256 - pos_x;
			if (flags & 2) {
				var14 = true;
				if (var16) {
					src += (sprite_w - 1) * sprite_h;
				} else {
					src += sprite_w - 1;
				}
			}
		}
	} else {
		sprite_clipped_w = pos_x + sprite_w;
		if (!(flags & 2)) {
			if (var16) {
				src -= sprite_h * pos_x;
				pos_x = 0;
			} else {
				src -= pos_x;
				pos_x = 0;
			}
		} else {
			var14 = true;
			if (var16) {
				src += sprite_h * (pos_x + sprite_w - 1);
				pos_x = 0;
			} else {
				src += pos_x + sprite_w - 1;
				var14 = true;
				pos_x = 0;
			}
		}
	}
	if (sprite_clipped_w <= 0) {
		return;
	}

	int16 sprite_clipped_h;
	if (pos_y >= 0) {
		if (pos_y < 224 - sprite_h) {
			sprite_clipped_h = sprite_h;
		} else {
			sprite_clipped_h = 224 - pos_y;
		}
	} else {
		sprite_clipped_h = sprite_h + pos_y;
		if (var16) {
			src -= pos_y;
		} else {
			src -= sprite_w * pos_y;
		}
		pos_y = 0;
	}
	if (sprite_clipped_h <= 0) {
		return;
	}

	if (!var14 && (flags & 2)) {
		if (var16) {
			src += sprite_h * (sprite_w - 1);
		} else {
			src += sprite_w - 1;
		}
	}

	uint32 dst_offset = 256 * pos_y + pos_x;
	uint8 sprite_col_mask = ((flags & 0x60) == 0x60) ? 0x50 : 0x40;

	debug(DBG_GAME, "dst_offset=0x%X src_offset=0x%X", dst_offset, src - dataPtr);
#if 0
	if (!(flags & 2)) {
		if (var16) {
			_vid.drawSpriteSub5(src, _vid._frontLayer + dst_offset, sprite_h, sprite_clipped_h, sprite_clipped_w, sprite_col_mask);
		} else {
			_vid.drawSpriteSub3(src, _vid._frontLayer + dst_offset, sprite_w, sprite_clipped_h, sprite_clipped_w, sprite_col_mask);
		}
	} else {
		if (var16) {
			_vid.drawSpriteSub6(src, _vid._frontLayer + dst_offset, sprite_h, sprite_clipped_h, sprite_clipped_w, sprite_col_mask);
		} else {
			_vid.drawSpriteSub4(src, _vid._frontLayer + dst_offset, sprite_w, sprite_clipped_h, sprite_clipped_w, sprite_col_mask);
		}
	}
	_vid.markBlockAsDirty(pos_x, pos_y, sprite_clipped_w, sprite_clipped_h);
#endif
}

uint8 *Game::loadBankData(uint16 mbkEntryNum) {
#if 0
	debug(DBG_GAME, "Game::loadBankData(%d)", mbkEntryNum);
	MbkEntry *me = &_res._mbk[mbkEntryNum];
	const uint16 avail = _lastBankData - _firstBankData;
	const uint16 size = (me->len & 0x7FFF) * 32;
	if (avail < size) {
		_curBankSlot = &_bankSlots[0];
		_curBankSlot->entryNum = 0xFFFF;
		_curBankSlot->ptr = 0;
		_firstBankData = _bankData;
	}
	_curBankSlot->entryNum = mbkEntryNum;
	_curBankSlot->ptr = _firstBankData;
	++_curBankSlot;
	_curBankSlot->entryNum = 0xFFFF;
	_curBankSlot->ptr = 0;
	const uint8 *data = _res._mbkData + me->offset;
	if (me->len & 0x8000) {
		warning("Uncompressed bank data %d", mbkEntryNum);
		memcpy(_firstBankData, data, size);
	} else {
		assert(me->offset != 0);
		if (!delphine_unpack(_firstBankData, data, 0)) {
			error("Bad CRC for bank data %d", mbkEntryNum);
		}
	}
	uint8 *bankData = _firstBankData;
	_firstBankData += size;
	assert(_firstBankData < _lastBankData);
	return bankData;
#endif
	return 0;
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
#if 0
		_res.load(_monsterNames[_curMonsterNum], Resource::OT_SPRM);
		_res.load_SPR_OFF(_monsterNames[_curMonsterNum], _res._sprm);
		_vid.setPaletteSlotLE(5, _monsterPals[_curMonsterNum]);
#endif
	}
	return 0xFFFF;
}

void Game::loadLevelMap() {
	debug(DBG_GAME, "Game::loadLevelMap() room=%d", _currentRoom);
	_currentIcon = 0xFF;
	_res.loadLevelRoom(_currentLevel, _currentRoom, _frontLayer, kScreenWidth);
	memcpy(_backLayer, _frontLayer, kScreenWidth * kScreenHeight);
	_res.setupLevelClut(_currentLevel, _palette);
}

void Game::loadLevelData() {
#if 0
	_res.clearLevelRes();

	const Level *lvl = &_gameLevels[_currentLevel];
	_res.load(lvl->name, Resource::OT_SPC);
	_res.load(lvl->name, Resource::OT_MBK);
	_res.load(lvl->name, Resource::OT_RP);
	_res.load(lvl->name, Resource::OT_CT);
	_res.load(lvl->name, Resource::OT_MAP);
	_res.load(lvl->name, Resource::OT_PAL);
	_res.load(lvl->name2, Resource::OT_PGE);
	_res.load(lvl->name2, Resource::OT_OBJ);
	_res.load(lvl->name2, Resource::OT_ANI);
	_res.load(lvl->name2, Resource::OT_TBN);

	_cut._id = lvl->cutscene_id;
#endif

	_res.loadLevelData(_currentLevel);

	_curMonsterNum = 0xFFFF;
	_curMonsterFrame = 0;

	_curBankSlot = &_bankSlots[0];
	_curBankSlot->entryNum = 0xFFFF;
	_curBankSlot->ptr = 0;
	_firstBankData = _bankData;
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

uint8 *Game::findBankData(uint16 entryNum) {
	BankSlot *slot = &_bankSlots[0];
	while (slot->entryNum != 0xFFFF) {
		if (slot->entryNum == entryNum) {
			return slot->ptr;
		}
		++slot;
	}
	return 0;
}

void Game::drawIcon(uint8 iconNum, int16 x, int16 y, uint8 colMask) {
#if 0
	uint16 offset = READ_BE_UINT16(_res._icn + iconNum * 2);
	uint8 buf[256];
	uint8 *p = _res._icn + offset + 2;
	for (int i = 0; i < 128; ++i) {
		uint8 col = *p++;
		buf[i * 2 + 0] = (col & 0xF0) >> 4;
		buf[i * 2 + 1] = (col & 0x0F) >> 0;
	}
	_vid.drawSpriteSub1(buf, _vid._frontLayer + x + y * 256, 16, 16, 16, colMask << 4);
	_vid.markBlockAsDirty(x, y, 16, 16);
#endif
	x *= 2;
	y *= 2;
	_res.decodeImageData(_res._icn, iconNum, _frontLayer + y * kScreenWidth + x, kScreenWidth);
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
#if 0
	_vid.setPalette0xF();
	_vid.setTextPalette();
	_vid.fullRefresh();
#endif
}

uint16 Game::getLineLength(const uint8 *str) const {
	uint16 len = 0;
	while (*str && *str != 0xB && *str != 0xA) {
		++str;
		++len;
	}
	return len;
}

void Game::handleInventory() {
#if 0
	LivePGE *selected_pge = 0;
	LivePGE *pge = &_pgeLive[0];
	if (pge->life > 0 && pge->current_inventory_PGE != 0xFF) {
		playSound(66, 0);
		InventoryItem items[24];
		int num_items = 0;
		uint8 inv_pge = pge->current_inventory_PGE;
		while (inv_pge != 0xFF) {
			items[num_items].icon_num = _res._pgeInit[inv_pge].icon_num;
			items[num_items].init_pge = &_res._pgeInit[inv_pge];
			items[num_items].live_pge = &_pgeLive[inv_pge];
			inv_pge = _pgeLive[inv_pge].next_inventory_PGE;
			++num_items;
		}
		items[num_items].icon_num = 0xFF;
		int current_item = 0;
		int num_lines = (num_items - 1) / 4 + 1;
		int current_line = 0;
		bool display_score = false;
		while (!_stub->_pi.backspace && !_stub->_pi.quit) {
			// draw inventory background
			int icon_h = 5;
			int icon_y = 140;
			int icon_num = 31;
			do {
				int icon_x = 56;
				int icon_w = 9;
				do {
					drawIcon(icon_num, icon_x, icon_y, 0xF);
					++icon_num;
					icon_x += 16;
				} while (--icon_w);
				icon_y += 16;
			} while (--icon_h);

			if (!display_score) {
				int icon_x_pos = 72;
				for (int i = 0; i < 4; ++i) {
					int item_it = current_line * 4 + i;
					if (items[item_it].icon_num == 0xFF) {
						break;
					}
					drawIcon(items[item_it].icon_num, icon_x_pos, 157, 0xA);
					if (current_item == item_it) {
						drawIcon(76, icon_x_pos, 157, 0xA);
						selected_pge = items[item_it].live_pge;
						uint8 txt_num = items[item_it].init_pge->text_num;
						const char *str = (const char *)_res._tbn + READ_BE_UINT16(_res._tbn + txt_num * 2);
						_vid.drawString(str, (256 - strlen(str) * 8) / 2, 189, 0xED);
						if (items[item_it].init_pge->init_flags & 4) {
							char counterValue[10];
							sprintf(counterValue, "%d", selected_pge->life);
							_vid.drawString(counterValue, (256 - strlen(counterValue) * 8) / 2, 197, 0xED);
						}
					}
					icon_x_pos += 32;
				}
				if (current_line != 0) {
					drawIcon(0x4E, 120, 176, 0xA); // down arrow
				}
				if (current_line != num_lines - 1) {
					drawIcon(0x4D, 120, 143, 0xA); // up arrow
				}
			} else {
				char textBuf[50];
				sprintf(textBuf, "SCORE %08u", _score);
				_vid.drawString(textBuf, (114 - strlen(textBuf) * 8) / 2 + 72, 158, 0xE5);
				sprintf(textBuf, "%s:%s", _res.getMenuString(LocaleData::LI_06_LEVEL), _res.getMenuString(LocaleData::LI_13_EASY + _skillLevel));
				_vid.drawString(textBuf, (114 - strlen(textBuf) * 8) / 2 + 72, 166, 0xE5);
			}

			_vid.updateScreen();
			_stub->sleep(80);
			inp_update();

			if (_stub->_pi.dirMask & PlayerInput::DIR_UP) {
				_stub->_pi.dirMask &= ~PlayerInput::DIR_UP;
				if (current_line < num_lines - 1) {
					++current_line;
					current_item = current_line * 4;
				}
			}
			if (_stub->_pi.dirMask & PlayerInput::DIR_DOWN) {
				_stub->_pi.dirMask &= ~PlayerInput::DIR_DOWN;
				if (current_line > 0) {
					--current_line;
					current_item = current_line * 4;
				}
			}
			if (_stub->_pi.dirMask & PlayerInput::DIR_LEFT) {
				_stub->_pi.dirMask &= ~PlayerInput::DIR_LEFT;
				if (current_item > 0) {
					int item_num = current_item % 4;
					if (item_num > 0) {
						--current_item;
					}
				}
			}
			if (_stub->_pi.dirMask & PlayerInput::DIR_RIGHT) {
				_stub->_pi.dirMask &= ~PlayerInput::DIR_RIGHT;
				if (current_item < num_items - 1) {
					int item_num = current_item % 4;
					if (item_num < 3) {
						++current_item;
					}
				}
			}
			if (_stub->_pi.enter) {
				_stub->_pi.enter = false;
				display_score = !display_score;
			}
		}
		_vid.fullRefresh();
		_stub->_pi.backspace = false;
		if (selected_pge) {
			pge_setCurrentInventoryObject(selected_pge);
		}
		playSound(66, 0);
	}
#endif
}

void Game::inp_update() {
//	_stub->processEvents();
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

