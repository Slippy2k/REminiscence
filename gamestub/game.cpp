
#include <ctime>
#include "file.h"
#include "unpack.h"
#include "game.h"

Game::Game(const char *dataPath, const char *savePath, ResourceType ver, Language lang)
	: _cut(&_pi, &_res, &_vid), _menu(&_res, &_vid), _res(dataPath, ver, lang), _sfx(&_mix), _vid(&_res),
	_dataPath(dataPath), _savePath(savePath) {
	memset(&_pi, 0, sizeof(_pi));
	_skillLevel = kDefaultSkill;
	_currentLevel = kDefaultLevel;
}

Game::~Game() {
	_res.free_TEXT();
	_mix.free();
}

int Game::getNextCutscene(int id) {
	if (id == 0x40) {
		return 0xD;
	}
	if (id == 0xD && !_cut._interrupted && _res._type == kResourceTypePC) {
		return 0x4A;
	}
	return 0xFFFF;
}

void Game::init() {

	_randSeed = time(0);

	_res.load_TEXT();

	_cut._id = 0xFFFF;

	switch (_res._type) {
	case kResourceTypeAmiga:
		_res.load("FONT8", Resource::OT_FNT, "SPR");
		break;
	case kResourceTypePC:
		_res.load("FB_TXT", Resource::OT_FNT);
		_res._hasSeqData = File().open("INTRO.SEQ", "rb", _dataPath);
		break;
	}

	switch (_res._type) {
	case kResourceTypeAmiga:
		_res.load("ICONE", Resource::OT_ICN, "SPR");
		_res.load("ICON", Resource::OT_ICN, "SPR");
		_res.load("PERSO", Resource::OT_SPM);
		break;
	case kResourceTypePC:
		_res.load("GLOBAL", Resource::OT_ICN);
		_res.load("GLOBAL", Resource::OT_SPC);
		_res.load("PERSO", Resource::OT_SPR);
		_res.load_SPR_OFF("PERSO", _res._spr1);
		_res.load_FIB("GLOBAL");
		break;
	}

	_cut._id = 0x40;

	_vid._unkPalSlot1 = 0;
	_vid._unkPalSlot2 = 0;
	_score = 0;
}

void Game::resetLevelState() {
	_animBuffers._states[0] = _animBuffer0State;
	_animBuffers._curPos[0] = 0xFF;
	_animBuffers._states[1] = _animBuffer1State;
	_animBuffers._curPos[1] = 0xFF;
	_animBuffers._states[2] = _animBuffer2State;
	_animBuffers._curPos[2] = 0xFF;
	_animBuffers._states[3] = _animBuffer3State;
	_animBuffers._curPos[3] = 0xFF;
	_currentRoom = _res._pgeInit[0].init_room;
	_cut._deathCutsceneId = 0xFFFF;
	_pge_opTempVar2 = 0xFFFF;
	_deathCutsceneCounter = 0;
	_saveStateCompleted = false;
	_loadMap = true;
	pge_resetGroups();
	_blinkingConradCounter = 0;
	_pge_processOBJ = false;
	_pge_opTempVar1 = 0;
	_textToDisplay = 0xFFFF;
	_gameOver = false;
}

void Game::continueGame() {
	_vid.fadeOut();
	if (_currentLevel == 7) {
		_vid.setTextPalette();
		playCutscene(0x3D);
		return;
	}
	_vid.setTextPalette();
	_vid.setPalette0xF();
	if (_validSaveState) {
		loadState();
	} else {
		loadLevelData();
	}
	resetLevelState();
}

void Game::doGame() {
/* TODO:
	if (_cut._id == 0x3D) {
		showFinalScore();
		break;
	}
*/
	if (_deathCutsceneCounter) {
		--_deathCutsceneCounter;
		if (_deathCutsceneCounter == 0) {
			_gameOver = true;
			playCutscene(_cut._deathCutsceneId);
			return;
		}
	}
	memcpy(_vid._frontLayer, _vid._backLayer, Video::GAMESCREEN_W * Video::GAMESCREEN_H);
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
			_cut._deathCutsceneId = 6;
			_deathCutsceneCounter = 1;
		} else {
			_currentRoom = _pgeLive[0].room_location;
			loadLevelMap();
			_loadMap = false;
			_vid.fullRefresh();
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
	_vid.updateScreen();
}

void Game::playCutscene(int id) {
	if (id != -1) {
		_cut._deathCutsceneId = 0xFFFF;
		_cut._id = id;
		_sfx.stop();
		if (_cut._id != 0x4A) {
// TODO:
//			_mod.play(Cutscene::_musicTable[_cut._id]);
		}
	}
}

void Game::drawCurrentInventoryItem() {
	uint16_t src = _pgeLive[0].current_inventory_PGE;
	if (src != 0xFF) {
		_currentIcon = _res._pgeInit[src].icon_num;
		drawIcon(_currentIcon, 232, 8, 0xA);
	}
}

void Game::showFinalScore() {
	// TODO:
}

void Game::initConfigPanel() {
	_configPanelItem = 0;
}

void Game::handleConfigPanel() {
	const int x = 7;
	const int y = 10;
	const int w = 17;
	const int h = 10;

	_vid._charShadowColor = 0xE2;
	_vid._charFrontColor = 0xEE;
	_vid._charTransparentColor = 0xFF;

	_vid.PC_drawChar(0x81, y, x);
	for (int i = 1; i < w; ++i) {
		_vid.PC_drawChar(0x85, y, x + i);
	}
	_vid.PC_drawChar(0x82, y, x + w);
	for (int j = 1; j < h; ++j) {
		_vid.PC_drawChar(0x86, y + j, x);
		for (int i = 1; i < w; ++i) {
			_vid._charTransparentColor = 0xE2;
			_vid.PC_drawChar(0x20, y + j, x + i);
		}
		_vid._charTransparentColor = 0xFF;
		_vid.PC_drawChar(0x87, y + j, x + w);
	}
	_vid.PC_drawChar(0x83, y + h, x);
	for (int i = 1; i < w; ++i) {
		_vid.PC_drawChar(0x88, y + h, x + i);
	}
	_vid.PC_drawChar(0x84, y + h, x + w);

	_menu._charVar3 = 0xE4;
	_menu._charVar4 = 0xE5;
	_menu._charVar1 = 0xE2;
	_menu._charVar2 = 0xEE;

	_menu.drawString(_res.getMenuString(LocaleData::LI_18_RESUME_GAME), y + 2, 9, _configPanelItem == 0 ? 2 : 3);
	_menu.drawString(_res.getMenuString(LocaleData::LI_20_LOAD_GAME), y + 4, 9, _configPanelItem == 1 ? 2 : 3);
	_menu.drawString(_res.getMenuString(LocaleData::LI_21_SAVE_GAME), y + 6, 9, _configPanelItem == 2 ? 2 : 3);
	_menu.drawString(_res.getMenuString(LocaleData::LI_19_ABORT_GAME), y + 8, 9, _configPanelItem == 3 ? 2 : 3);

	if (_pi.dirMask & PlayerInput::DIR_UP) {
		_pi.dirMask &= ~PlayerInput::DIR_UP;
		_configPanelItem = (_configPanelItem + 3) % 4;
	}
	if (_pi.dirMask & PlayerInput::DIR_DOWN) {
		_pi.dirMask &= ~PlayerInput::DIR_DOWN;
		_configPanelItem = (_configPanelItem + 1) % 4;
	}
}

void Game::initContinueAbort() {
	_cut._id = 0x48;
	_cut.initCutscene();
	_continueAbortItem = 0;
	_continueAbortCounter = 100;
	_vid.getPaletteEntry(0xE4, &_continueAbortColor);
	_continueAbortColorInc = 0xFF;
}

void Game::handleContinueAbort() {
	if (!_cut._stop) {
		_cut.playCutscene();
		return;
	}
	memcpy(_vid._frontLayer, _cut._page0, Video::GAMESCREEN_W * Video::GAMESCREEN_H);
	static const uint8_t colors[] = { 0xE4, 0xE5 };

	const char *str = _res.getMenuString(LocaleData::LI_01_CONTINUE_OR_ABORT);
	_vid.drawString(str, (256 - strlen(str) * 8) / 2, 64, 0xE3);
	str = _res.getMenuString(LocaleData::LI_02_TIME);
	char buf[50];
	snprintf(buf, sizeof(buf), "%s : %d", str, _continueAbortCounter / 10);
	_vid.drawString(buf, 96, 88, 0xE3);
	str = _res.getMenuString(LocaleData::LI_03_CONTINUE);
	_vid.drawString(str, (256 - strlen(str) * 8) / 2, 104, colors[_continueAbortItem]);
	str = _res.getMenuString(LocaleData::LI_04_ABORT);
	_vid.drawString(str, (256 - strlen(str) * 8) / 2, 112, colors[_continueAbortItem ^ 1]);
	snprintf(buf, sizeof(buf), "SCORE  %08u", _score);
	_vid.drawString(buf, 64, 154, 0xE3);

	if (_continueAbortColor.b >= 0x3D) {
		_continueAbortColorInc = 0;
	} else if (_continueAbortColor.b < 2) {
		_continueAbortColorInc = 0xFF;
	}
	if (_continueAbortColorInc == 0xFF) {
		_continueAbortColor.b += 2;
		_continueAbortColor.g += 2;
	} else {
		_continueAbortColor.b -= 2;
		_continueAbortColor.g -= 2;
	}
	_vid.setPaletteEntry(0xE4, &_continueAbortColor);

	--_continueAbortCounter;

	if (_pi.dirMask & PlayerInput::DIR_UP) {
		_pi.dirMask &= ~PlayerInput::DIR_UP;
		if (_continueAbortItem != 0) {
			_continueAbortItem = 0;
		}
	}
	if (_pi.dirMask & PlayerInput::DIR_DOWN) {
		_pi.dirMask &= ~PlayerInput::DIR_DOWN;
		if (_continueAbortItem != 1) {
			_continueAbortItem = 1;
		}
	}
}

void Game::printLevelCode() {
	if (_printLevelCodeCounter != 0) {
		--_printLevelCodeCounter;
		if (_printLevelCodeCounter != 0) {
			char buf[32];
			snprintf(buf, sizeof(buf), "CODE: %s", _passwords[_currentLevel][_skillLevel]);
			_vid.drawString(buf, (Video::GAMESCREEN_W - strlen(buf) * 8) / 2, 16, 0xE7);
		}
	}
}

void Game::printSaveStateCompleted() {
	if (_saveStateCompleted) {
		const char *str = _res.getMenuString(LocaleData::LI_05_COMPLETED);
		_vid.drawString(str, (176 - strlen(str) * 8) / 2, 34, 0xE6);
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
			uint8_t icon_num = obj - 1;
			drawIcon(icon_num, 80, 8, 0xA);
			uint8_t txt_num = pge->init_PGE->text_num;
			const char *str = (const char *)_res._tbn + READ_LE_UINT16(_res._tbn + txt_num * 2);
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
}

void Game::initStoryTexts() {
	_textStoryOffset = 0;
	memcpy(_vid._tempLayer, _vid._frontLayer, Video::GAMESCREEN_W * Video::GAMESCREEN_H);
}

void Game::handleStoryTexts() {
	memcpy(_vid._frontLayer, _vid._tempLayer, Video::GAMESCREEN_W * Video::GAMESCREEN_H);
	uint16_t text_col_mask = 0xE8;
	const uint8_t *str = _res.getGameString(_textToDisplay) + _textStoryOffset;
	drawIcon(_currentInventoryIconNum, 80, 8, 0xA);
	int nextOffset = _textStoryOffset;
	if (*str == 0xFF) {
		text_col_mask = READ_LE_UINT16(str + 1);
		str += 3;
		nextOffset += 3;
	}
	int16_t text_y_pos = 26;
	while (1) {
		uint16_t len = getLineLength(str);
		_vid.drawString((const char *)str, (176 - len * 8) / 2, text_y_pos, text_col_mask);
		text_y_pos += 8;
		nextOffset += len + 1;
		str += len;
		if (*str == 0 || *str == 0xB) {
			break;
		}
		++str;
	}
	if (_pi.backspace) {
		_pi.backspace = false;
		_textStoryOffset = nextOffset;
		if (*str == 0) {
			_textToDisplay = 0xFFFF;
		}
	}
}

void Game::prepareAnims() {
	if (!(_currentRoom & 0x80) && _currentRoom < 0x40) {
		int8_t pge_room;
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

void Game::prepareAnimsHelper(LivePGE *pge, int16_t dx, int16_t dy) {
	debug(DBG_GAME, "Game::prepareAnimsHelper() dx=0x%X dy=0x%X pge_num=%d pge->flags=0x%X pge->anim_number=0x%X", dx, dy, pge - &_pgeLive[0], pge->flags, pge->anim_number);
	if (!(pge->flags & 8)) {
		if (pge->index != 0 && loadMonsterSprites(pge) == 0) {
			return;
		}
		assert(pge->anim_number < 1287);
		const uint8_t *dataPtr = _res._spr_off[pge->anim_number];
		if (dataPtr == 0) {
			return;
		}
		const int8_t dw = (int8_t)dataPtr[0];
		const int8_t dh = (int8_t)dataPtr[1];
		uint8_t w = 0, h = 0;
		switch (_res._type) {
		case kResourceTypeAmiga:
			w = ((dataPtr[2] >> 7) + 1) * 16;
			h = dataPtr[2] & 0x7F;
			break;
		case kResourceTypePC:
			w = dataPtr[2];
			h = dataPtr[3];
			dataPtr += 4;
			break;
		}
		const int16_t ypos = dy + pge->pos_y - dh + 2;
		int16_t xpos = dx + pge->pos_x - dw;
		if (pge->flags & 2) {
			xpos = dw + dx + pge->pos_x;
			uint8_t _cl = w;
			if (_cl & 0x40) {
				_cl = h;
			} else {
				_cl &= 0x3F;
			}
			xpos -= _cl;
		}
		if (xpos <= -32 || xpos >= 256 || ypos < -48 || ypos >= 224) {
			return;
		}
		xpos += 8;
		if (pge == &_pgeLive[0]) {
			_animBuffers.addState(1, xpos, ypos, dataPtr, pge, w, h);
		} else if (pge->flags & 0x10) {
			_animBuffers.addState(2, xpos, ypos, dataPtr, pge, w, h);
		} else {
			_animBuffers.addState(0, xpos, ypos, dataPtr, pge, w, h);
		}
	} else {
		assert(pge->anim_number < _res._numSpc);
		const uint8_t *dataPtr = _res._spc + READ_BE_UINT16(_res._spc + pge->anim_number * 2);
		const int16_t xpos = dx + pge->pos_x + 8;
		const int16_t ypos = dy + pge->pos_y + 2;
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

void Game::drawAnimBuffer(uint8_t stateNum, AnimBufferState *state) {
	debug(DBG_GAME, "Game::drawAnimBuffer() state=%d", stateNum);
	assert(stateNum < 4);
	_animBuffers._states[stateNum] = state;
	uint8_t lastPos = _animBuffers._curPos[stateNum];
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
				switch (_res._type) {
				case kResourceTypeAmiga:
					_vid.AMIGA_decodeSpm(state->dataPtr, _res._memBuf);
					drawCharacter(_res._memBuf, state->x, state->y, state->h, state->w, pge->flags);
					break;
				case kResourceTypePC:
					if (!(state->dataPtr[-2] & 0x80)) {
						decodeCharacterFrame(state->dataPtr, _res._memBuf);
						drawCharacter(_res._memBuf, state->x, state->y, state->h, state->w, pge->flags);
					} else {
						drawCharacter(state->dataPtr, state->x, state->y, state->h, state->w, pge->flags);
					}
					break;
				}
			} else {
				drawObject(state->dataPtr, state->x, state->y, pge->flags);
			}
			--state;
		} while (--numAnims != 0);
	}
}

void Game::drawObject(const uint8_t *dataPtr, int16_t x, int16_t y, uint8_t flags) {
	debug(DBG_GAME, "Game::drawObject() dataPtr[]=0x%X dx=%d dy=%d",  dataPtr[0], (int8_t)dataPtr[1], (int8_t)dataPtr[2]);
	assert(dataPtr[0] < 0x4A);
	uint8_t slot = _res._rp[dataPtr[0]];
	uint8_t *data = _res.findBankData(slot);
	if (data == 0) {
		data = _res.loadBankData(slot);
	}
	int16_t posy = y - (int8_t)dataPtr[2];
	int16_t posx = x;
	if (flags & 2) {
		posx += (int8_t)dataPtr[1];
	} else {
		posx -= (int8_t)dataPtr[1];
	}
	int count = 0;
	switch (_res._type) {
	case kResourceTypeAmiga:
		count = dataPtr[8];
		dataPtr += 9;
		break;
	case kResourceTypePC:
		count = dataPtr[5];
		dataPtr += 6;
		break;
	}
	for (int i = 0; i < count; ++i) {
		drawObjectFrame(data, dataPtr, posx, posy, flags);
		dataPtr += 4;
	}
}

void Game::drawObjectFrame(const uint8_t *bankDataPtr, const uint8_t *dataPtr, int16_t x, int16_t y, uint8_t flags) {
	debug(DBG_GAME, "Game::drawObjectFrame(0x%X, %d, %d, 0x%X)", dataPtr, x, y, flags);
	const uint8_t *src = bankDataPtr + dataPtr[0] * 32;

	int16_t sprite_y = y + dataPtr[2];
	int16_t sprite_x;
	if (flags & 2) {
		sprite_x = x - dataPtr[1] - (((dataPtr[3] & 0xC) + 4) * 2);
	} else {
		sprite_x = x + dataPtr[1];
	}

	uint8_t sprite_flags = dataPtr[3];
	if (flags & 2) {
		sprite_flags ^= 0x10;
	}

	uint8_t sprite_h = (((sprite_flags >> 0) & 3) + 1) * 8;
	uint8_t sprite_w = (((sprite_flags >> 2) & 3) + 1) * 8;

	switch (_res._type) {
	case kResourceTypeAmiga:
		if (sprite_w == 24) {
			// TODO: fix p24xN
			return;
		}
		_vid.AMIGA_decodeSpc(src, sprite_w, sprite_h, _res._memBuf);
		break;
	case kResourceTypePC:
		_vid.PC_decodeSpc(src, sprite_w, sprite_h, _res._memBuf);
		break;
	}

	src = _res._memBuf;
	bool sprite_mirror_x = false;
	int16_t sprite_clipped_w;
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

	int16_t sprite_clipped_h;
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

	uint32_t dst_offset = 256 * sprite_y + sprite_x;
	uint8_t sprite_col_mask = (flags & 0x60) >> 1;

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
}

void Game::decodeCharacterFrame(const uint8_t *dataPtr, uint8_t *dstPtr) {
	int n = READ_BE_UINT16(dataPtr); dataPtr += 2;
	uint16_t len = n * 2;
	uint8_t *dst = dstPtr + 0x400;
	while (n--) {
		uint8_t c = *dataPtr++;
		dst[0] = (c & 0xF0) >> 4;
		dst[1] = (c & 0x0F) >> 0;
		dst += 2;
	}
	dst = dstPtr;
	const uint8_t *src = dstPtr + 0x400;
	do {
		uint8_t c1 = *src++;
		if (c1 == 0xF) {
			uint8_t c2 = *src++;
			uint16_t c3 = *src++;
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

void Game::drawCharacter(const uint8_t *dataPtr, int16_t pos_x, int16_t pos_y, uint8_t a, uint8_t b, uint8_t flags) {
	debug(DBG_GAME, "Game::drawCharacter(0x%X, %d, %d, 0x%X, 0x%X, 0x%X)", dataPtr, pos_x, pos_y, a, b, flags);

	bool var16 = false; // sprite_mirror_y
	if (b & 0x40) {
		b &= 0xBF;
		SWAP(a, b);
		var16 = true;
	}
	uint16_t sprite_h = a;
	uint16_t sprite_w = b;

	const uint8_t *src = dataPtr;
	bool var14 = false;

	int16_t sprite_clipped_w;
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

	int16_t sprite_clipped_h;
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

	uint32_t dst_offset = 256 * pos_y + pos_x;
	uint8_t sprite_col_mask = ((flags & 0x60) == 0x60) ? 0x50 : 0x40;

	debug(DBG_GAME, "dst_offset=0x%X src_offset=0x%X", dst_offset, src - dataPtr);

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
		if (_res._type == kResourceTypeAmiga) {
			_res.load(_monsterNames[1][_curMonsterNum], Resource::OT_SPM);
			static const uint8_t tab[4] = { 0, 8, 0, 8 };
			const int offset = _vid._mapPalSlot2 * 16 + tab[_curMonsterNum];
			for (int i = 0; i < 8; ++i) {
				_vid.setPaletteColorBE(0x50 + i, offset + i);
			}
		} else {
			const char *name = _monsterNames[0][_curMonsterNum];
			_res.load(name, Resource::OT_SPRM);
			_res.load_SPR_OFF(name, _res._sprm);
			_vid.setPaletteSlotLE(5, _monsterPals[_curMonsterNum]);
		}
	}
	return 0xFFFF;
}

void Game::loadLevelMap() {
	debug(DBG_GAME, "Game::loadLevelMap() room=%d", _currentRoom);
	_currentIcon = 0xFF;
	switch (_res._type) {
	case kResourceTypeAmiga:
		if (_currentLevel == 1) {
			static const uint8_t tab[64] = {
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1, 0,
				0, 0, 0, 1, 0, 0, 0, 0, 2, 0, 0, 2, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 1, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0
			};
			const int num = tab[_currentRoom];
			if (num != 0 && _res._levNum != num) {
				char name[8];
				snprintf(name, sizeof(name), "level2_%d", num);
				_res.load(name, Resource::OT_LEV);
				_res._levNum = num;
			}
		}
		_vid.AMIGA_decodeLev(_currentLevel, _currentRoom);
		break;
	case kResourceTypePC:
		_vid.PC_decodeMap(_currentLevel, _currentRoom);
		_vid.PC_setLevelPalettes();
		break;
	}
}

void Game::loadLevelData() {
	_res.clearLevelRes();
	const Level *lvl = &_gameLevels[_currentLevel];
	switch (_res._type) {
	case kResourceTypeAmiga:
		{
			const char *name = lvl->nameAmiga;
			if (_currentLevel == 4) {
				name = _gameLevels[3].nameAmiga;
			}
			_res.load(name, Resource::OT_MBK);
			if (_currentLevel == 6) {
				_res.load(_gameLevels[5].nameAmiga, Resource::OT_CT);
			} else {
				_res.load(name, Resource::OT_CT);
			}
			_res.load(name, Resource::OT_PAL);
			_res.load(name, Resource::OT_RPC);
			_res.load(name, Resource::OT_SPC);
			if (_currentLevel == 1) {
				_res.load("level2_1", Resource::OT_LEV);
				_res._levNum = 1;
			} else {
				_res.load(name, Resource::OT_LEV);
			}
		}
		_res.load(lvl->nameAmiga, Resource::OT_PGE);
		_res.load(lvl->nameAmiga, Resource::OT_OBC);
		_res.load(lvl->nameAmiga, Resource::OT_ANI);
		_res.load(lvl->nameAmiga, Resource::OT_TBN);
		{
			char name[8];
			snprintf(name, sizeof(name), "level%d", lvl->sound);
			_res.load(name, Resource::OT_SPL);
		}
		if (_currentLevel == 0) {
			_res.load(lvl->nameAmiga, Resource::OT_SGD);
		}
		break;
	case kResourceTypePC:
		_res.load(lvl->name, Resource::OT_MBK);
		_res.load(lvl->name, Resource::OT_CT);
		_res.load(lvl->name, Resource::OT_PAL);
		_res.load(lvl->name, Resource::OT_RP);
		_res.load(lvl->name, Resource::OT_MAP);
		_res.load(lvl->name2, Resource::OT_PGE);
		_res.load(lvl->name2, Resource::OT_OBJ);
		_res.load(lvl->name2, Resource::OT_ANI);
		_res.load(lvl->name2, Resource::OT_TBN);
		break;
	}

	_cut._id = lvl->cutscene_id;

	_curMonsterNum = 0xFFFF;
	_curMonsterFrame = 0;

	_res.clearBankData();
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
	_validSaveState = false;
}

void Game::drawIcon(uint8_t iconNum, int16_t x, int16_t y, uint8_t colMask) {
	uint8_t buf[16 * 16];
	switch (_res._type) {
	case kResourceTypeAmiga:
		if (iconNum > 30) {
			// inventory icons
			switch (iconNum) {
			case 76: // cursor
				memset(buf, 0, 16 * 16);
				for (int i = 0; i < 3; ++i) {
					buf[i] = buf[15 * 16 + (15 - i)] = 1;
					buf[i * 16] = buf[(15 - i) * 16 + 15] = 1;
				}
				break;
			case 77: // up - icon.spr 4
				memset(buf, 0, 16 * 16);
				_vid.AMIGA_decodeIcn(_res._icn, 35, buf);
				break;
			case 78: // down - icon.spr 5
				memset(buf, 0, 16 * 16);
				_vid.AMIGA_decodeIcn(_res._icn, 36, buf);
				break;
			default:
				memset(buf, 5, 16 * 16);
				break;
			}
		} else {
			_vid.AMIGA_decodeIcn(_res._icn, iconNum, buf);
		}
		break;
	case kResourceTypePC:
		_vid.PC_decodeIcn(_res._icn, iconNum, buf);
		break;
	}
	_vid.drawSpriteSub1(buf, _vid._frontLayer + x + y * 256, 16, 16, 16, colMask << 4);
	_vid.markBlockAsDirty(x, y, 16, 16);
}

void Game::playSound(uint8_t sfxId, uint8_t softVol) {
	if (sfxId < _res._numSfx) {
		SoundFx *sfx = &_res._sfxList[sfxId];
		if (sfx->data) {
			MixerChunk mc;
			mc.data = sfx->data;
			mc.len = sfx->len;
			const int freq = _res._type == kResourceTypeAmiga ? 3546897 / 650 : 6000;
			_mix.play(&mc, freq, Mixer::MAX_VOLUME >> softVol);
		}
	} else {
		// in-game music
		_sfx.play(sfxId);
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
	_vid.fadeOut();
	loadLevelData();
	loadLevelMap();
	_vid.setPalette0xF();
	_vid.setTextPalette();
	_vid.fullRefresh();
}

uint16_t Game::getLineLength(const uint8_t *str) const {
	uint16_t len = 0;
	while (*str && *str != 0xB && *str != 0xA) {
		++str;
		++len;
	}
	return len;
}

void Game::initInventory() {
	_inventoryCurrentItem = 0;
	_inventoryItemsCount = 0;
	int i = _pgeLive[0].current_inventory_PGE;
	while (i != 0xFF) {
		_inventoryItems[_inventoryItemsCount].icon_num = _res._pgeInit[i].icon_num;
		_inventoryItems[_inventoryItemsCount].init_pge = &_res._pgeInit[i];
		_inventoryItems[_inventoryItemsCount].live_pge = &_pgeLive[i];
		++_inventoryItemsCount;
		i = _pgeLive[i].next_inventory_PGE;
	}
	_inventoryItems[_inventoryItemsCount].icon_num = 0xFF;
}

void Game::handleInventory() {
	const int h = (_inventoryItemsCount - 1) / 4 + 1;
	const int y = _inventoryCurrentItem / 4;
	if (_pi.dirMask & PlayerInput::DIR_UP) {
		_pi.dirMask &= ~PlayerInput::DIR_UP;
		if (y < h - 1) {
			_inventoryCurrentItem = (y + 1) * 4;
		}
	}
	if (_pi.dirMask & PlayerInput::DIR_DOWN) {
		_pi.dirMask &= ~PlayerInput::DIR_DOWN;
		if (y > 0) {
			_inventoryCurrentItem = (y - 1) * 4;
		}
	}
	if (_pi.dirMask & PlayerInput::DIR_LEFT) {
		_pi.dirMask &= ~PlayerInput::DIR_LEFT;
		if (_inventoryCurrentItem > 0) {
			const int num = _inventoryCurrentItem % 4;
			if (num > 0) {
				--_inventoryCurrentItem;
			}
		}
	}
	if (_pi.dirMask & PlayerInput::DIR_RIGHT) {
		_pi.dirMask &= ~PlayerInput::DIR_RIGHT;
		if (_inventoryCurrentItem < _inventoryItemsCount - 1) {
			const int num = _inventoryCurrentItem % 4;
			if (num < 3) {
				++_inventoryCurrentItem;
			}
		}
	}
	if (_pi.backspace) {
		pge_setCurrentInventoryObject(_inventoryItems[_inventoryCurrentItem].live_pge);
	}
	int iconNum = 31;
	for (int y = 0; y < 5; ++y) {
		for (int x = 0; x < 9; ++x) {
			drawIcon(iconNum, 56 + x * 16, 140 + y * 16, 0xF);
			++iconNum;
		}
	}
	if (_pi.enter) {
		char buf[50];
		snprintf(buf, sizeof(buf), "SCORE %08u", _score);
		_vid.drawString(buf, (114 - strlen(buf) * 8) / 2 + 72, 158, 0xE5);
		snprintf(buf, sizeof(buf), "%s:%s", _res.getMenuString(LocaleData::LI_06_LEVEL), _res.getMenuString(LocaleData::LI_13_EASY + _skillLevel));
		_vid.drawString(buf, (114 - strlen(buf) * 8) / 2 + 72, 166, 0xE5);
		return;
	}
	for (int i = 0; i < 4; ++i) {
		int currentItem = y * 4 + i;
		if (_inventoryItems[currentItem].icon_num == 0xFF) {
			break;
		}
		const int xPos = 72 + i * 32;
		drawIcon(_inventoryItems[currentItem].icon_num, xPos, 157, 0xA);
		if (_inventoryCurrentItem == currentItem) {
			drawIcon(76, xPos, 157, 0xA);
			const LivePGE *selected_pge = _inventoryItems[currentItem].live_pge;
			uint8_t txt_num = _inventoryItems[currentItem].init_pge->text_num;
			const char *str = (const char *)_res._tbn + READ_LE_UINT16(_res._tbn + txt_num * 2);
			_vid.drawString(str, (256 - strlen(str) * 8) / 2, 189, 0xED);
			if (_inventoryItems[currentItem].init_pge->init_flags & 4) {
				char buf[10];
				snprintf(buf, sizeof(buf), "%d", selected_pge->life);
				_vid.drawString(buf, (256 - strlen(buf) * 8) / 2, 197, 0xED);
			}
		}
	}
	if (y != 0) {
		drawIcon(78, 120, 176, 0xA); // down arrow
	}
	if (y != h - 1) {
		drawIcon(77, 120, 143, 0xA); // up arrow
	}
}

void AnimBuffers::addState(uint8_t stateNum, int16_t x, int16_t y, const uint8_t *dataPtr, LivePGE *pge, uint8_t w, uint8_t h) {
	debug(DBG_GAME, "AnimBuffers::addState() stateNum=%d x=%d y=%d dataPtr=0x%X pge=0x%X", stateNum, x, y, dataPtr, pge);
	assert(stateNum < 4);
	AnimBufferState *state = _states[stateNum];
	state->x = x;
	state->y = y;
	state->w = w;
	state->h = h;
	state->dataPtr = dataPtr;
	state->pge = pge;
	++_curPos[stateNum];
	++_states[stateNum];
}
