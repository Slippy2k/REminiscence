/* REminiscence - Flashback interpreter
 * Copyright (C) 2005-2010 Gregory Montoir
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "file.h"
#include "game.h"
#include "systemstub.h"

static const char *USAGE =
	"REminiscence - Flashback Interpreter\n"
	"Usage: %s [OPTIONS]...\n"
	"  --datapath=PATH   Path to data files (default 'DATA')\n"
	"  --savepath=PATH   Path to save files (default '.')";

static bool parseOption(const char *arg, const char *longCmd, const char **opt) {
	bool handled = false;
	if (arg[0] == '-' && arg[1] == '-') {
		if (strncmp(arg + 2, longCmd, strlen(longCmd)) == 0) {
			*opt = arg + 2 + strlen(longCmd);
			handled = true;
		}
	}
	return handled;
}

static Version detectVersion(const char *dataPath) {
	static struct {
		const char *filename;
		Version ver;
	} checkTable[] = {
		{ "ENGCINE.BIN", VER_EN },
		{ "FR_CINE.BIN", VER_FR },
		{ "GERCINE.BIN", VER_DE },
		{ "SPACINE.BIN", VER_SP }
	};
	for (uint8 i = 0; i < ARRAYSIZE(checkTable); ++i) {
		File f;
		if (f.open(checkTable[i].filename, dataPath, "rb")) {
			return checkTable[i].ver;
		}
	}
	error("Unable to find data files, check that all required files are present");
	return VER_EN;
}

#undef main
int main(int argc, char *argv[]) {
	const char *dataPath = "DATA";
	const char *savePath = ".";
	for (int i = 1; i < argc; ++i) {
		bool opt = false;
		if (strlen(argv[i]) >= 2) {
			opt |= parseOption(argv[i], "datapath=", &dataPath);
			opt |= parseOption(argv[i], "savepath=", &savePath);
		}
		if (!opt) {
			printf(USAGE, argv[0]);
			return 0;
		}
	}
	Version ver = detectVersion(dataPath);
	g_debugMask = DBG_INFO; // DBG_CUT | DBG_VIDEO | DBG_RES | DBG_MENU | DBG_PGE | DBG_GAME | DBG_UNPACK | DBG_COL | DBG_MOD | DBG_SFX;
	SystemStub *stub = SystemStub_SDL_create();
	Game *g = new Game(stub, dataPath, savePath, ver);
	g->run();
	delete g;
	delete stub;
	return 0;
}
