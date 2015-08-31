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

#include <sys/stat.h>
#include <cstdio>
#include "cutscene.h"
#include "systemstub.h"

static const char *USAGE = 
	"fbply - Flashback Cutscene Player\n"
	"Usage: fbply FILE NUM\n";

struct Parameters {
	const char *path;
	const char *name;
	int num;
};

static void parsePath(const char *path, Parameters *params) {
	char *p = strdup(path);
	if (p) {
		struct stat st;
		if (stat(path, &st) == 0) {
			if (S_ISDIR(st.st_mode)) {
				params->path = p;
				params->name = 0;
			} else if (S_ISREG(st.st_mode)) {
				char *sep = strrchr(p, '/');
				if (sep) {
					*sep = 0;
					params->path = p;
					params->name = sep + 1;
					sep = strrchr(sep + 1, '.');
					if (sep) {
						*sep = 0;
					}
				}
			}
		}
	}
}

static int playCutscene(const Parameters *params, Cutscene *cut) {
	SystemStub *stub = SystemStub_OGL_create();
	stub->init("Flashback Cutscene Player");
	cut->init(stub, params->path);
	int index;
	if (params->name) {
		cut->_cutId = 0xFFFF;
		cut->_cutName = params->name;
		cut->load();
		index = params->num;
	} else {
		cut->_cutId = params->num;
		index = cut->load();
	}
	cut->main(index);
	stub->destroy();
	delete stub;
	return 0;
}

int main(int argc, char *argv[]) {
	Parameters params;
	Cutscene cut;

	memset(&params, 0, sizeof(params));
	memset(&cut, 0, sizeof(cut));

	g_debugMask = 0; // DBG_CUTSCENE;
	if (argc == 2 || argc == 3) {
		if (argc == 3) {
			params.num = atoi(argv[2]);
		}
		parsePath(argv[1], &params);
		return playCutscene(&params, &cut);
	}
	printf("%s\n", USAGE);
	printf("\nAvailable cutscenes :\n");
	Cutscene::dumpCutNum();
	return 0;
}
