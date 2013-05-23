
#ifndef MENU_H__
#define MENU_H__

#include "intern.h"

#define kDefaultLevel 0
#define kDefaultSkill 1

struct Resource;
struct Video;

struct Menu {
	enum {
		MENU_OPTION_ITEM_START,
		MENU_OPTION_ITEM_SKILL,
		MENU_OPTION_ITEM_LEVEL,
		MENU_OPTION_ITEM_INFO,
		MENU_OPTION_ITEM_QUIT
	};
	enum {
		SCREEN_TITLE,
		SCREEN_SKILL,
		SCREEN_LEVEL,
		SCREEN_INFO
	};

	enum {
		EVENTS_DELAY = 80
	};

	Resource *_res;
	Video *_vid;

	const char **_textOptions;
	uint8_t _charVar1;
	uint8_t _charVar2;
	uint8_t _charVar3;
	uint8_t _charVar4;
	uint8_t _charVar5;

	int _currentScreen;
	int _newScreen;
	int _currentOption;
	int _selectedOption;

	int _level;
	int _skill;

	Menu(Resource *res, Video *vid);

	void drawString(const char *str, int16_t y, int16_t x, uint8_t color);
	void drawString2(const char *str, int16_t y, int16_t x);
	void loadPicture(const char *prefix);

	void handleInfoScreen(PlayerInput *pi);
	void handleSkillScreen(PlayerInput *pi);
	void handleLevelScreen(PlayerInput *pi);
	void handleTitleScreen(PlayerInput *pi);

	void initMenu();
	void handleMenu(PlayerInput *pi);
};

#endif // MENU_H__
