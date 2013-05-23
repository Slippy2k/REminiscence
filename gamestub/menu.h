
#ifndef MENU_H__
#define MENU_H__

#include "intern.h"

struct Resource;
struct Video;

struct Menu {
	enum {
		MENU_OPTION_ITEM_START,
		MENU_OPTION_ITEM_SKILL,
		MENU_OPTION_ITEM_PASSWORD,
		MENU_OPTION_ITEM_LEVEL,
		MENU_OPTION_ITEM_INFO,
		MENU_OPTION_ITEM_QUIT
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

	Menu(Resource *res, Video *vid);

	void drawString(const char *str, int16_t y, int16_t x, uint8_t color);
	void drawString2(const char *str, int16_t y, int16_t x);
	void loadPicture(const char *prefix);
	void handleInfoScreen();
	void handleSkillScreen(uint8_t &new_skill);
	bool handlePasswordScreen(uint8_t &new_skill, uint8_t &new_level);
	bool handleLevelScreen(uint8_t &new_skill, uint8_t &new_level);
	bool handleTitleScreen(uint8_t &new_skill, uint8_t &new_level);
};

#endif // MENU_H__
