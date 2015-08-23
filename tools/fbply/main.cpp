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

#include <cstdio>
#include "cutscene.h"
#include "systemstub.h"

static const char *USAGE = 
	"fbply - Flashback Cutscene Player\n"
	"Usage: fbply [OPTIONS]... \n"
	"  --datapath=PATH   Path to where the datafiles are (default '.')\n"
	"  --cutscene=NUM    Cutscene number to play\n";

static void parseOption(const char *arg, const char *longCmd, const char **opt) {
	if (arg[0] == '-' && arg[1] == '-') {
		if (strncmp(arg + 2, longCmd, strlen(longCmd)) == 0) {
			*opt = arg + 2 + strlen(longCmd);
		}
	}
}

#undef main
int main(int argc, char *argv[]) {
	const char *dataPath = ".";
	const char *cutName = 0;
	const char *cutId = NULL;
	for (int i = 1; i < argc; ++i) {
		parseOption(argv[i], "datapath=", &dataPath);
		parseOption(argv[i], "cutscene=", &cutId);
		parseOption(argv[i], "name=", &cutName);
	}
	if (cutId || cutName) {
		SystemStub *stub = SystemStub_OGL_create();
		g_debugMask = 0; //DBG_CUTSCENE; // | DBG_SYSSTUB;
		stub->init("Flashback Cutscene Player");
		Cutscene cp;
		memset(&cp, 0, sizeof(cp));
		cp.init(stub, dataPath);
		if (cutId) {
			cp._cutId = strtol(cutId, NULL, 0);
		}
		if (cutName) {
			cp._cutId = 0xFFFF;
			cp._cutName = cutName;
		}
		cp.start();
		stub->destroy();
		delete stub;
	} else {
		printf("%s\n", USAGE);
		printf("\nAvailable cutscenes :\n");
		Cutscene::dumpCutNum();
	}
	return 0;
}
