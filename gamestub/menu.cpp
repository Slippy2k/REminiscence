
#include "game.h"
#include "resource.h"
#include "video.h"
#include "menu.h"

Menu::Menu(Resource *res, Video *vid)
	: _res(res), _vid(vid) {
	_currentScreen = -1;
	_newScreen = SCREEN_TITLE;
	_level = kDefaultLevel;
	_skill = kDefaultSkill;
}

void Menu::drawString(const char *str, int16_t y, int16_t x, uint8_t color) {
	debug(DBG_MENU, "Menu::drawString()");
	uint8_t v1b = _vid->_charFrontColor;
	uint8_t v2b = _vid->_charTransparentColor;
	uint8_t v3b = _vid->_charShadowColor;
	switch (color) {
	case 0:
		_vid->_charFrontColor = _charVar1;
		_vid->_charTransparentColor = _charVar2;
		_vid->_charShadowColor = _charVar2;
		break;
	case 1:
		_vid->_charFrontColor = _charVar2;
		_vid->_charTransparentColor = _charVar1;
		_vid->_charShadowColor = _charVar1;
		break;
	case 2:
		_vid->_charFrontColor = _charVar3;
		_vid->_charTransparentColor = 0xFF;
		_vid->_charShadowColor = _charVar1;
		break;
	case 3:
		_vid->_charFrontColor = _charVar4;
		_vid->_charTransparentColor = 0xFF;
		_vid->_charShadowColor = _charVar1;
		break;
	case 4:
		_vid->_charFrontColor = _charVar2;
		_vid->_charTransparentColor = 0xFF;
		_vid->_charShadowColor = _charVar1;
		break;
	case 5:
		_vid->_charFrontColor = _charVar2;
		_vid->_charTransparentColor = 0xFF;
		_vid->_charShadowColor = _charVar5;
		break;
	}

	drawString2(str, y, x);

	_vid->_charFrontColor = v1b;
	_vid->_charTransparentColor = v2b;
	_vid->_charShadowColor = v3b;
}

void Menu::drawString2(const char *str, int16_t y, int16_t x) {
	debug(DBG_MENU, "Menu::drawString2()");
	int len = 0;
	while (*str) {
		_vid->PC_drawChar((uint8_t)*str, y, x + len);
		++str;
		++len;
	}
	_vid->markBlockAsDirty(x * 8, y * 8, len * 8, 8);
}

void Menu::loadPicture(const char *prefix) {
	debug(DBG_MENU, "Menu::loadPicture('%s')", prefix);
	_res->load_MAP_menu(prefix, _res->_memBuf);
	for (int i = 0; i < 4; ++i) {
		for (int y = 0; y < 224; ++y) {
			for (int x = 0; x < 64; ++x) {
				_vid->_backLayer[i + x * 4 + 256 * y] = _res->_memBuf[0x3800 * i + x + 64 * y];
			}
		}
	}
	_res->load_PAL_menu(prefix, _res->_memBuf);
	const uint8_t *p = _res->_memBuf;
	for (int i = 0; i < 256; ++i) {
		Color c;
		c.r = p[0] >> 2;
		c.g = p[1] >> 2;
		c.b = p[2] >> 2;
		p += 3;
		_vid->setPaletteEntry(i, &c);
	}
}

void Menu::handleInfoScreen(PlayerInput *pi) {
	debug(DBG_MENU, "Menu::handleInfoScreen()");
	if (pi->enter) {
		pi->enter = false;
		_newScreen = SCREEN_TITLE;
	}
}

void Menu::handleSkillScreen(PlayerInput *pi) {
	debug(DBG_MENU, "Menu::handleSkillScreen()");
	static const uint8_t option_colors[3][3] = { { 2, 3, 3 }, { 3, 2, 3}, { 3, 3, 2 } };
	drawString(_res->getMenuString(LocaleData::LI_12_SKILL_LEVEL), 12, 4, 3);
	drawString(_res->getMenuString(LocaleData::LI_13_EASY), 15, 14, option_colors[_skill][0]);
	drawString(_res->getMenuString(LocaleData::LI_14_NORMAL), 17, 14, option_colors[_skill][1]);
	drawString(_res->getMenuString(LocaleData::LI_15_EXPERT), 19, 14, option_colors[_skill][2]);

	if (pi->dirMask & PlayerInput::DIR_UP) {
		pi->dirMask &= ~PlayerInput::DIR_UP;
		if (_skill != 0) {
			--_skill;
		} else {
			_skill = 2;
		}
	}
	if (pi->dirMask & PlayerInput::DIR_DOWN) {
		pi->dirMask &= ~PlayerInput::DIR_DOWN;
		if (_skill != 2) {
			++_skill;
		} else {
			_skill = 0;
		}
	}
	if (pi->enter) {
		pi->enter = false;
		_newScreen = SCREEN_TITLE;
	}
}

void Menu::handleLevelScreen(PlayerInput *pi) {
	debug(DBG_MENU, "Menu::handleLevelScreen()");
	static const char *levelTitles[] = {
		"Titan / The Jungle",
		"Titan / New Washington",
		"Titan / Death Tower Show",
		"Earth / Surface",
		"Earth / Paradise Club",
		"Planet Morphs / Surface",
		"Planet Morphs / Inner Core"
	};
	for (int i = 0; i < 7; ++i) {
		drawString(levelTitles[i], 7 + i * 2, 4, (_level == i) ? 2 : 3);
	}

	drawString(_res->getMenuString(LocaleData::LI_13_EASY),   23,  4, (_skill == 0) ? 2 : 3);
	drawString(_res->getMenuString(LocaleData::LI_14_NORMAL), 23, 14, (_skill == 1) ? 2 : 3);
	drawString(_res->getMenuString(LocaleData::LI_15_EXPERT), 23, 24, (_skill == 2) ? 2 : 3);

	if (pi->dirMask & PlayerInput::DIR_UP) {
		pi->dirMask &= ~PlayerInput::DIR_UP;
		if (_level != 0) {
			--_level;
		} else {
			_level = 6;
		}
	}
	if (pi->dirMask & PlayerInput::DIR_DOWN) {
		pi->dirMask &= ~PlayerInput::DIR_DOWN;
		if (_level != 6) {
			++_level;
		} else {
			_level = 0;
		}
	}
	if (pi->dirMask & PlayerInput::DIR_LEFT) {
		pi->dirMask &= ~PlayerInput::DIR_LEFT;
		if (_skill != 0) {
			--_skill;
		} else {
			_skill = 2;
		}
	}
	if (pi->dirMask & PlayerInput::DIR_RIGHT) {
		pi->dirMask &= ~PlayerInput::DIR_RIGHT;
		if (_skill != 2) {
			++_skill;
		} else {
			_skill = 0;
		}
	}
	if (pi->enter) {
		pi->enter = false;
		_newScreen = SCREEN_TITLE;
	}
}

void Menu::handleTitleScreen(PlayerInput *pi) {
	static const struct {
		int str;
		int opt;
	} menu_items[] = {
		{ LocaleData::LI_07_START, MENU_OPTION_ITEM_START },
		{ LocaleData::LI_06_LEVEL, MENU_OPTION_ITEM_LEVEL },
		{ LocaleData::LI_10_INFO, MENU_OPTION_ITEM_INFO },
		{ LocaleData::LI_11_QUIT, MENU_OPTION_ITEM_QUIT }
	};
	static const int menu_items_count = ARRAYSIZE(menu_items);

	const int y_start = 26 - menu_items_count * 2;
	for (int i = 0; i < menu_items_count; ++i) {
		drawString(_res->getMenuString(menu_items[i].str), y_start + i * 2, 20, (i == _currentOption) ? 2 : 3);
	}

	if (pi->dirMask & PlayerInput::DIR_UP) {
		pi->dirMask &= ~PlayerInput::DIR_UP;
		if (_currentOption != 0) {
			--_currentOption;
		} else {
			_currentOption = menu_items_count - 1;
		}
	}
	if (pi->dirMask & PlayerInput::DIR_DOWN) {
		pi->dirMask &= ~PlayerInput::DIR_DOWN;
		if (_currentOption != menu_items_count - 1) {
			++_currentOption;
		} else {
			_currentOption = 0;
		}
	}
	if (pi->enter) {
		pi->enter = false;
		_selectedOption = menu_items[_currentOption].opt;
		switch (_selectedOption) {
		case MENU_OPTION_ITEM_START:
			break;
		case MENU_OPTION_ITEM_SKILL:
			_newScreen = SCREEN_SKILL;
			break;
		case MENU_OPTION_ITEM_LEVEL:
			_newScreen = SCREEN_LEVEL;
			break;
		case MENU_OPTION_ITEM_INFO:
			_newScreen = SCREEN_INFO;
			break;
		case MENU_OPTION_ITEM_QUIT:
			break;
		}
	}
}

void Menu::initMenu() {
	switch (_currentScreen) {
	case SCREEN_TITLE:
		loadPicture("menu1");
		_charVar1 = 0;
		_charVar2 = 0;
		_charVar3 = 1;
		_charVar4 = 2;
		_charVar5 = 0;
		break;
	case SCREEN_SKILL:
		loadPicture("menu3");
		break;
	case SCREEN_LEVEL:
		loadPicture("menu2");
		break;
	case SCREEN_INFO:
		if (_res->_lang == LANG_FR) {
			loadPicture("instru_f");
		} else {
			loadPicture("instru_e");
		}
		break;
	}
	_selectedOption = -1;
}

void Menu::handleMenu(PlayerInput *pi) {
	if (_currentScreen != _newScreen) {
		_currentScreen = _newScreen;
		initMenu();
	}
	memcpy(_vid->_frontLayer, _vid->_backLayer, Video::GAMESCREEN_W * Video::GAMESCREEN_H);
	switch (_currentScreen) {
	case SCREEN_TITLE:
		handleTitleScreen(pi);
		break;
	case SCREEN_SKILL:
		handleSkillScreen(pi);
		break;
	case SCREEN_LEVEL:
		handleLevelScreen(pi);
		break;
	case SCREEN_INFO:
		handleInfoScreen(pi);
		break;
	}
}
