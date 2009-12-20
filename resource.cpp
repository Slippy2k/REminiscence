/* REminiscence - Flashback interpreter
 * Copyright (C) 2005-2009 Gregory Montoir
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
#include "unpack.h"
#include "resource.h"


Resource::Resource(const char *dataPath, Version ver) {
	memset(this, 0, sizeof(Resource));
	_dataPath = dataPath;
	_ver = ver;
	_useAmigaData = false;
	_memBuf = (uint8 *)malloc(0xE000);
}

Resource::~Resource() {
	clearLevelRes();
	free(_fnt);
	free(_icn);
	free(_tab);
	free(_spr1);
	free(_memBuf);
	free(_cmd);
	free(_pol);
	free(_cine_off);
	free(_cine_txt);
	for (int i = 0; i < _numSfx; ++i) {
		free(_sfxList[i].data);
	}
	free(_sfxList);
	free(_voiceBuf);
}

void Resource::clearLevelRes() {
	free(_tbn); _tbn = 0;
	free(_mbk); _mbk = 0;
	free(_mbkData); _mbkData = 0;
	free(_pal); _pal = 0;
	free(_map); _map = 0;
	free(_spc); _spc = 0;
	free(_ani); _ani = 0;
	free_OBJ();
}

void Resource::load_FIB(const char *fileName) {
	debug(DBG_RES, "Resource::load_FIB('%s')", fileName);
	static const uint8 fibonacciTable[] = {
		0xDE, 0xEB, 0xF3, 0xF8, 0xFB, 0xFD, 0xFE, 0xFF,
		0x00, 0x01, 0x02, 0x03, 0x05, 0x08, 0x0D, 0x15
	};
	snprintf(_entryName, sizeof(_entryName), "%s.FIB", fileName);
	File f;
	if (f.open(_entryName, _dataPath, "rb")) {
		_numSfx = f.readUint16LE();
		_sfxList = (SoundFx *)malloc(_numSfx * sizeof(SoundFx));
		if (!_sfxList) {
			error("Unable to allocate SoundFx table");
		}
		int i;
		for (i = 0; i < _numSfx; ++i) {
			SoundFx *sfx = &_sfxList[i];
			sfx->offset = f.readUint32LE();
			sfx->len = f.readUint16LE();
			sfx->data = 0;
		}
		for (i = 0; i < _numSfx; ++i) {
			SoundFx *sfx = &_sfxList[i];
			if (sfx->len == 0) {
				continue;
			}
			f.seek(sfx->offset);
			uint8 *data = (uint8 *)malloc(sfx->len * 2);
			if (!data) {
				error("Unable to allocate SoundFx data buffer");
			}
			sfx->data = data;
			uint8 c = f.readByte();
			*data++ = c;
			*data++ = c;
			uint16 sz = sfx->len - 1;
			while (sz--) {
				uint8 d = f.readByte();
				c += fibonacciTable[d >> 4];
				*data++ = c;
				c += fibonacciTable[d & 15];
				*data++ = c;
			}
			sfx->len *= 2;
		}
		if (f.ioErr()) {
			error("I/O error when reading '%s'", _entryName);
		}
	} else {
		error("Can't open '%s'", _entryName);
	}
}

void Resource::load_MAP_menu(const char *fileName, uint8 *dstPtr) {
	debug(DBG_RES, "Resource::load_MAP_menu('%s')", fileName);
	snprintf(_entryName, sizeof(_entryName), "%s.MAP", fileName);
	File f;
	if (f.open(_entryName, _dataPath, "rb")) {
		if (f.size() != 0x3800 * 4) {
			error("Wrong file size for '%s', %d", _entryName, f.size());
		}
		f.read(dstPtr, 0x3800 * 4);
		if (f.ioErr()) {
			error("I/O error when reading '%s'", _entryName);
		}
	} else {
		error("Can't open '%s'", _entryName);
	}
}

void Resource::load_PAL_menu(const char *fileName, uint8 *dstPtr) {
	debug(DBG_RES, "Resource::load_PAL_menu('%s')", fileName);
	snprintf(_entryName, sizeof(_entryName), "%s.PAL", fileName);
	File f;
	if (f.open(_entryName, _dataPath, "rb")) {
		if (f.size() != 768) {
			error("Wrong file size for '%s', %d", _entryName, f.size());
		}
		f.read(dstPtr, 768);
		if (f.ioErr()) {
			error("I/O error when reading '%s'", _entryName);
		}
	} else {
		error("Can't open '%s'", _entryName);
	}
}

void Resource::load_SPR_OFF(const char *fileName, uint8 *sprData) {
	debug(DBG_RES, "Resource::load_SPR_OFF('%s')", fileName);
	snprintf(_entryName, sizeof(_entryName), "%s.OFF", fileName);
	File f;
	if (f.open(_entryName, _dataPath, "rb")) {
		int len = f.size();
		uint8 *offData = (uint8 *)malloc(len);
		if (!offData) {
			error("Unable to allocate sprite offsets");
		}
		f.read(offData, len);
		if (f.ioErr()) {
			error("I/O error when reading '%s'", _entryName);
		}
		const uint8 *p = offData;
		uint16 pos;
		while ((pos = READ_LE_UINT16(p)) != 0xFFFF) {
			uint32 off = READ_LE_UINT32(p + 2);
			if (off == 0xFFFFFFFF) {
				_spr_off[pos] = 0;
			} else {
				_spr_off[pos] = sprData + off;
			}
			p += 6;
		}
		free(offData);
	} else {
		error("Can't open '%s'", _entryName);
	}
}

void Resource::load_CINE() {
	const char *baseName = 0;
	switch (_ver) {
	case VER_FR:
		baseName = "FR_CINE";
		break;
	case VER_EN:
		baseName = "ENGCINE";
		break;
	case VER_DE:
		baseName = "GERCINE";
		break;
	case VER_SP:
		baseName = "SPACINE";
		break;
	}
	debug(DBG_RES, "Resource::load_CINE('%s')", baseName);
	if (_cine_off == 0) {
		snprintf(_entryName, sizeof(_entryName), "%s.BIN", baseName);
		File f;
		if (f.open(_entryName, _dataPath, "rb")) {
			int len = f.size();
			_cine_off = (uint8 *)malloc(len);
			if (!_cine_off) {
				error("Unable to allocate cinematics offsets");
			}
			f.read(_cine_off, len);
			if (f.ioErr()) {
				error("I/O error when reading '%s'", _entryName);
			}
		} else {
			error("Can't open '%s'", _entryName);
		}
	}
	if (_cine_txt == 0) {
		snprintf(_entryName, sizeof(_entryName), "%s.TXT", baseName);
		File f;
		if (f.open(_entryName, _dataPath, "rb")) {
			int len = f.size();
			_cine_txt = (uint8 *)malloc(len);
			if (!_cine_txt) {
				error("Unable to allocate cinematics text data");
			}
			f.read(_cine_txt, len);
			if (f.ioErr()) {
				error("I/O error when reading '%s'", _entryName);
			}
		} else {
			error("Can't open '%s'", _entryName);
		}
	}
}

void Resource::load_TEXT() {
	File f;
	// Load game strings
	_stringsTable = 0;
	if (f.open("STRINGS.TXT", _dataPath, "rb")) {
		const int sz = f.size();
		_extStringsTable = (uint8 *)malloc(sz);
		if (_extStringsTable) {
			f.read(_extStringsTable, sz);
			_stringsTable = _extStringsTable;
		}
		f.close();
	}
	if (!_stringsTable) {
		switch (_ver) {
		case VER_FR:
			_stringsTable = LocaleData::_stringsTableFR;
			break;
		case VER_EN:
			_stringsTable = LocaleData::_stringsTableEN;
			break;
		case VER_DE:
			_stringsTable = LocaleData::_stringsTableDE;
			break;
		case VER_SP:
			_stringsTable = LocaleData::_stringsTableSP;
			break;
		}
	}
	// Load menu strings
	_textsTable = 0;
	if (f.open("MENUS.TXT", _dataPath, "rb")) {
		const int offs = LocaleData::LI_NUM * sizeof(char *);
		const int sz = f.size() + 1;
		_extTextsTable = (char **)malloc(offs + sz);
		if (_extTextsTable) {
			char *textData = (char *)_extTextsTable + offs;
			f.read(textData, sz);
			textData[sz] = 0;
			int textsCount = 0;
			for (char *eol; (eol = strpbrk(textData, "\r\n")) != 0; ) {
				*eol++ = 0;
				if (*eol == '\r' || *eol == '\n') {
					*eol++ = 0;
				}
				if (textsCount < LocaleData::LI_NUM && textData[0] != 0) {
					_extTextsTable[textsCount] = textData;
					++textsCount;
				}
				textData = eol;
			}
			if (textsCount < LocaleData::LI_NUM && textData[0] != 0) {
				_extTextsTable[textsCount] = textData;
				++textsCount;
			}
			if (textsCount < LocaleData::LI_NUM) {
				free(_extTextsTable);
				_extTextsTable = 0;
			} else {
				_textsTable = (const char **)_extTextsTable;
			}
		}
	}
	if (!_textsTable) {
		switch (_ver) {
		case VER_FR:
			_textsTable = LocaleData::_textsTableFR;
			break;
		case VER_EN:
			_textsTable = LocaleData::_textsTableEN;
			break;
		case VER_DE:
			_textsTable = LocaleData::_textsTableDE;
			break;
		case VER_SP:
			_textsTable = LocaleData::_textsTableSP;
			break;
		}
	}
}

void Resource::free_TEXT() {
	if (_extTextsTable) {
		free(_extTextsTable);
		_extTextsTable = 0;
	}
	_stringsTable = 0;
	if (_extStringsTable) {
		free(_extStringsTable);
		_extStringsTable = 0;
	}
	_textsTable = 0;
}

void Resource::load(const char *objName, int objType) {
	debug(DBG_RES, "Resource::load('%s', %d)", objName, objType);
	LoadStub loadStub = 0;
	switch (objType) {
	case OT_MBK:
		snprintf(_entryName, sizeof(_entryName), "%s.MBK", objName);
		loadStub = &Resource::load_MBK;
		break;
	case OT_PGE:
		snprintf(_entryName, sizeof(_entryName), "%s.PGE", objName);
		loadStub = &Resource::load_PGE;
		break;
	case OT_PAL:
		snprintf(_entryName, sizeof(_entryName), "%s.PAL", objName);
		loadStub = &Resource::load_PAL;
		break;
	case OT_CT:
		snprintf(_entryName, sizeof(_entryName), "%s.CT", objName);
		loadStub = &Resource::load_CT;
		break;
	case OT_MAP:
		debug(DBG_RES, "ouverture map (map)");
		snprintf(_entryName, sizeof(_entryName), "%s.MAP", objName);
		loadStub = &Resource::load_MAP;
		break;
	case OT_SPC:
		strcpy(_entryName, "GLOBAL.SPC");
		loadStub = &Resource::load_SPC;
		break;
	case OT_RP:
		snprintf(_entryName, sizeof(_entryName), "%s.RP", objName);
		loadStub = &Resource::load_RP;
		break;
	case OT_SPR:
		snprintf(_entryName, sizeof(_entryName), "%s.SPR", objName);
		loadStub = &Resource::load_SPR;
		break;
	case OT_SPRM:
		snprintf(_entryName, sizeof(_entryName), "%s.SPR", objName);
		loadStub = &Resource::load_SPRM;
		break;
	case OT_ICN:
		snprintf(_entryName, sizeof(_entryName), "%s.ICN", objName);
		loadStub = &Resource::load_ICN;
		break;
	case OT_FNT:
		snprintf(_entryName, sizeof(_entryName), "%s.FNT", objName);
		loadStub = &Resource::load_FNT;
		break;
	case OT_OBJ:
		snprintf(_entryName, sizeof(_entryName), "%s.OBJ", objName);
		loadStub = &Resource::load_OBJ;
		break;
	case OT_ANI:
		snprintf(_entryName, sizeof(_entryName), "%s.ANI", objName);
		loadStub = &Resource::load_ANI;
		break;
	case OT_TBN:
		snprintf(_entryName, sizeof(_entryName), "%s.TBN", objName);
		loadStub = &Resource::load_TBN;
		break;
	case OT_CMD:
		snprintf(_entryName, sizeof(_entryName), "%s.CMD", objName);
		loadStub = &Resource::load_CMD;
		break;
	case OT_POL:
		snprintf(_entryName, sizeof(_entryName), "%s.POL", objName);
		loadStub = &Resource::load_POL;
		break;
	case OT_CMP:
		snprintf(_entryName, sizeof(_entryName), "%s.CMP", objName);
		loadStub = &Resource::load_CMP;
		break;
	case OT_OBC:
		snprintf(_entryName, sizeof(_entryName), "%s.OBC", objName);
		loadStub = &Resource::load_OBC;
		break;
	default:
		error("Unimplemented Resource::load() type %d", objType);
		break;
	}
	if (loadStub) {
		File f;
		if (f.open(_entryName, _dataPath, "rb")) {
			(this->*loadStub)(&f);
			if (f.ioErr()) {
				error("I/O error when reading '%s'", _entryName);
			}
		} else {
			error("Can't open '%s'", _entryName);
		}
	}
}

void Resource::load_CT(File *pf) {
	debug(DBG_RES, "Resource::load_CT()");
	int len = pf->size();
	uint8 *tmp = (uint8 *)malloc(len);
	if (!tmp) {
		error("Unable to allocate CT buffer");
	} else {
		pf->read(tmp, len);
		if (!delphine_unpack((uint8 *)_ctData, tmp, len)) {
			error("Bad CRC for collision data");
		}
		free(tmp);
	}
}

void Resource::load_FNT(File *f) {
	debug(DBG_RES, "Resource::load_FNT()");
	int len = f->size();
	_fnt = (uint8 *)malloc(len);
	if (!_fnt) {
		error("Unable to allocate FNT buffer");
	} else {
		f->read(_fnt, len);
	}
}

void Resource::load_MBK(File *f) {
	debug(DBG_RES, "Resource::load_MBK()");
	uint8 num = f->readByte();
	int dataSize = f->size() - num * 6;
	_mbk = (MbkEntry *)malloc(sizeof(MbkEntry) * num);
	if (!_mbk) {
		error("Unable to allocate MBK buffer");
	}
	f->seek(0);
	for (int i = 0; i < num; ++i) {
		f->readUint16BE(); /* unused */
		_mbk[i].offset = f->readUint16BE() - num * 6;
		_mbk[i].len = f->readUint16BE();
		debug(DBG_RES, "dataSize=0x%X entry %d off=0x%X len=0x%X", dataSize, i, _mbk[i].offset + num * 6, _mbk[i].len);
		assert(_mbk[i].offset <= dataSize);
	}
	_mbkData = (uint8 *)malloc(dataSize);
	if (!_mbkData) {
		error("Unable to allocate MBK data buffer");
	}
	f->read(_mbkData, dataSize);
}

void Resource::load_ICN(File *f) {
	debug(DBG_RES, "Resource::load_ICN()");
	int len = f->size();
	_icn = (uint8 *)malloc(len);
	if (!_icn) {
		error("Unable to allocate ICN buffer");
	} else {
		f->read(_icn, len);
	}
}

void Resource::load_SPR(File *f) {
	debug(DBG_RES, "Resource::load_SPR()");
	int len = f->size() - 12;
	_spr1 = (uint8 *)malloc(len);
	if (!_spr1) {
		error("Unable to allocate SPR buffer");
	} else {
		f->seek(12);
		f->read(_spr1, len);
	}
}

void Resource::load_SPRM(File *f) {
	debug(DBG_RES, "Resource::load_SPRM()");
	int len = f->size() - 12;
	f->seek(12);
	f->read(_sprm, len);
}

void Resource::load_RP(File *f) {
	debug(DBG_RES, "Resource::load_RP()");
	f->read(_rp, 0x4A);
}

void Resource::load_SPC(File *f) {
	debug(DBG_RES, "Resource::load_SPC()");
	int len = f->size();
	_spc = (uint8 *)malloc(len);
	if (!_spc) {
		error("Unable to allocate SPC buffer");
	} else {
		f->read(_spc, len);
		_numSpc = READ_BE_UINT16(_spc) / 2;
	}
}

void Resource::load_PAL(File *f) {
	debug(DBG_RES, "Resource::load_PAL()");
	int len = f->size();
	_pal = (uint8 *)malloc(len);
	if (!_pal) {
		error("Unable to allocate PAL buffer");
	} else {
		f->read(_pal, len);
	}
}

void Resource::load_MAP(File *f) {
	debug(DBG_RES, "Resource::load_MAP()");
	int len = f->size();
	_map = (uint8 *)malloc(len);
	if (!_map) {
		error("Unable to allocate MAP buffer");
	} else {
		f->read(_map, len);
	}
}

void Resource::load_OBJ(File *f) {
	debug(DBG_RES, "Resource::load_OBJ()");
	_numObjectNodes = f->readUint16LE();
	assert(_numObjectNodes < 255);
	uint32 offsets[256];
	for (int i = 0; i < _numObjectNodes; ++i) {
		offsets[i] = f->readUint32LE();
	}
	offsets[_numObjectNodes] = f->size() - 2;
	int numObjectsCount = 0;
	uint16 objectsCount[256];
	for (int i = 0; i < _numObjectNodes; ++i) {
		int diff = offsets[i + 1] - offsets[i];
		if (diff != 0) {
			objectsCount[numObjectsCount] = (diff - 2) / 0x12;
			debug(DBG_RES, "i=%d objectsCount[numObjectsCount]=%d", i, objectsCount[numObjectsCount]);
			++numObjectsCount;
		}
	}
	uint32 prevOffset = 0;
	ObjectNode *prevNode = 0;
	int iObj = 0;
	for (int i = 0; i < _numObjectNodes; ++i) {
		if (prevOffset != offsets[i]) {
			ObjectNode *on = (ObjectNode *)malloc(sizeof(ObjectNode));
			if (!on) {
				error("Unable to allocate ObjectNode num=%d", i);
			}
			f->seek(offsets[i] + 2);
			on->last_obj_number = f->readUint16LE();
			on->num_objects = objectsCount[iObj];
			debug(DBG_RES, "last=%d num=%d", on->last_obj_number, on->num_objects);
			on->objects = (Object *)malloc(sizeof(Object) * on->num_objects);
			for (int j = 0; j < on->num_objects; ++j) {
				Object *obj = &on->objects[j];
				obj->type = f->readUint16LE();
				obj->dx = f->readByte();
				obj->dy = f->readByte();
				obj->init_obj_type = f->readUint16LE();
				obj->opcode2 = f->readByte();
				obj->opcode1 = f->readByte();
				obj->flags = f->readByte();
				obj->opcode3 = f->readByte();
				obj->init_obj_number = f->readUint16LE();
				obj->opcode_arg1 = f->readUint16LE();
				obj->opcode_arg2 = f->readUint16LE();
				obj->opcode_arg3 = f->readUint16LE();
				debug(DBG_RES, "obj_node=%d obj=%d op1=0x%X op2=0x%X op3=0x%X", i, j, obj->opcode2, obj->opcode1, obj->opcode3);
			}
			++iObj;
			prevOffset = offsets[i];
			prevNode = on;
		}
		_objectNodesMap[i] = prevNode;
	}
}

void Resource::free_OBJ() {
	debug(DBG_RES, "Resource::free_OBJ()");
	ObjectNode *prevNode = 0;
	for (int i = 0; i < _numObjectNodes; ++i) {
		if (_objectNodesMap[i] != prevNode) {
			ObjectNode *curNode = _objectNodesMap[i];
			free(curNode->objects);
			free(curNode);
			prevNode = curNode;
		}
		_objectNodesMap[i] = 0;
	}
}

void Resource::load_OBC(File *f) {
	const int packedSize = f->readUint32BE();
	uint8 *packedData = (uint8 *)malloc(packedSize);
	if (!packedData) {
		error("Unable to allocate OBC temporary buffer 1");
	}
	f->seek(packedSize);
	const int unpackedSize = f->readUint32BE();
	uint8 *tmp = (uint8 *)malloc(unpackedSize);
	if (!tmp) {
		error("Unable to allocate OBC temporary buffer 2");
	}
	f->seek(4);
	f->read(packedData, packedSize);
	if (!delphine_unpack(tmp, packedData, packedSize)) {
		error("Bad CRC for compressed object data");
	}
	free(packedData);
	uint32 offsets[256];
	int tmpOffset = 0;
	_numObjectNodes = 230;
	for (int i = 0; i < _numObjectNodes; ++i) {
		offsets[i] = READ_BE_UINT32(tmp + tmpOffset); tmpOffset += 4;
	}
	offsets[_numObjectNodes] = unpackedSize;
	int numObjectsCount = 0;
	uint16 objectsCount[256];
	for (int i = 0; i < _numObjectNodes; ++i) {
		int diff = offsets[i + 1] - offsets[i];
		if (diff != 0) {
			objectsCount[numObjectsCount] = (diff - 2) / 0x12;
			++numObjectsCount;
		}
	}
	uint32 prevOffset = 0;
	ObjectNode *prevNode = 0;
	int iObj = 0;
	for (int i = 0; i < _numObjectNodes; ++i) {
		if (prevOffset != offsets[i]) {
			ObjectNode *on = (ObjectNode *)malloc(sizeof(ObjectNode));
			if (!on) {
				error("Unable to allocate ObjectNode num=%d", i);
			}
			const uint8 *objData = tmp + offsets[i];
			on->last_obj_number = READ_BE_UINT16(objData); objData += 2;
			on->num_objects = objectsCount[iObj];
			on->objects = (Object *)malloc(sizeof(Object) * on->num_objects);
			for (int j = 0; j < on->num_objects; ++j) {
				Object *obj = &on->objects[j];
				obj->type = READ_BE_UINT16(objData); objData += 2;
				obj->dx = *objData++;
				obj->dy = *objData++;
				obj->init_obj_type = READ_BE_UINT16(objData); objData += 2;
				obj->opcode2 = *objData++;
				obj->opcode1 = *objData++;
				obj->flags = *objData++;
				obj->opcode3 = *objData++;
				obj->init_obj_number = READ_BE_UINT16(objData); objData += 2;
				obj->opcode_arg1 = READ_BE_UINT16(objData); objData += 2;
				obj->opcode_arg2 = READ_BE_UINT16(objData); objData += 2;
				obj->opcode_arg3 = READ_BE_UINT16(objData); objData += 2;
				debug(DBG_RES, "obj_node=%d obj=%d op1=0x%X op2=0x%X op3=0x%X", i, j, obj->opcode2, obj->opcode1, obj->opcode3);
			}
			++iObj;
			prevOffset = offsets[i];
			prevNode = on;
		}
		_objectNodesMap[i] = prevNode;
	}
	free(tmp);
}

void Resource::load_PGE(File *f) {
	debug(DBG_RES, "Resource::load_PGE()");
	int len = f->size() - 2;
	_pgeNum = f->readUint16LE();
	memset(_pgeInit, 0, sizeof(_pgeInit));
	debug(DBG_RES, "len=%d _pgeNum=%d", len, _pgeNum);
	for (uint16 i = 0; i < _pgeNum; ++i) {
		InitPGE *pge = &_pgeInit[i];
		pge->type = f->readUint16LE();
		pge->pos_x = f->readUint16LE();
		pge->pos_y = f->readUint16LE();
		pge->obj_node_number = f->readUint16LE();
		pge->life = f->readUint16LE();
		for (int lc = 0; lc < 4; ++lc) {
			pge->counter_values[lc] = f->readUint16LE();
		}
		pge->object_type = f->readByte();
		pge->init_room = f->readByte();
		pge->room_location = f->readByte();
		pge->init_flags = f->readByte();
		pge->colliding_icon_num = f->readByte();
		pge->icon_num = f->readByte();
		pge->object_id = f->readByte();
		pge->skill = f->readByte();
		pge->mirror_x = f->readByte();
		pge->flags = f->readByte();
		pge->unk1C = f->readByte();
		f->readByte();
		pge->text_num = f->readByte();
		f->readByte();
	}
}

void Resource::load_ANI(File *f) {
	debug(DBG_RES, "Resource::load_ANI()");
	int size = f->size() - 2;
	_ani = (uint8 *)malloc(size);
	if (!_ani) {
		error("Unable to allocate ANI buffer");
	} else {
		f->seek(2);
		f->read(_ani, size);
	}
}

void Resource::load_TBN(File *f) {
	debug(DBG_RES, "Resource::load_TBN()");
	int len = f->size();
	_tbn = (uint8 *)malloc(len);
	if (!_tbn) {
		error("Unable to allocate TBN buffer");
	} else {
		f->read(_tbn, len);
	}
}

void Resource::load_CMD(File *pf) {
	debug(DBG_RES, "Resource::load_CMD()");
	free(_cmd);
	int len = pf->size();
	_cmd = (uint8 *)malloc(len);
	if (!_cmd) {
		error("Unable to allocate CMD buffer");
	} else {
		pf->read(_cmd, len);
	}
}

void Resource::load_POL(File *pf) {
	debug(DBG_RES, "Resource::load_POL()");
	free(_pol);
	int len = pf->size();
	_pol = (uint8 *)malloc(len);
	if (!_pol) {
		error("Unable to allocate POL buffer");
	} else {
		pf->read(_pol, len);
	}
}

void Resource::load_CMP(File *pf) {
	free(_pol);
	free(_cmd);
	int len = pf->size();
	uint8 *tmp = (uint8 *)malloc(len);
	if (!tmp) {
		error("Unable to allocate CMP buffer");
	}
	pf->read(tmp, len);
	struct {
		int offset, packedSize, size;
	} data[2];
	int offset = 0;
	for (int i = 0; i < 2; ++i) {
		int packedSize = READ_BE_UINT32(tmp + offset); offset += 4;
		assert((packedSize & 1) == 0);
		if (packedSize < 0) {
			data[i].size = packedSize = -packedSize;
		} else {
			data[i].size = READ_BE_UINT32(tmp + offset + packedSize - 4);
		}
		data[i].offset = offset;
		data[i].packedSize = packedSize;
		offset += packedSize;
	}
	_pol = (uint8 *)malloc(data[0].size);
	if (!_pol) {
		error("Unable to allocate POL buffer");
	}
	if (data[0].packedSize == data[0].size) {
		memcpy(_pol, tmp + data[0].offset, data[0].packedSize);
	} else if (!delphine_unpack(_pol, tmp + data[0].offset, data[0].packedSize)) {
		error("Bad CRC for cutscene polygon data");
	}
	_cmd = (uint8 *)malloc(data[1].size);
	if (!_cmd) {
		error("Unable to allocate CMD buffer");
	}
	if (data[1].packedSize == data[1].size) {
		memcpy(_cmd, tmp + data[1].offset, data[1].packedSize);
	} else if (!delphine_unpack(_cmd, tmp + data[1].offset, data[1].packedSize)) {
		error("Bad CRC for cutscene command data");
	}
	free(tmp);
}

void Resource::load_VCE(int num, int segment, uint8 **buf, uint32 *bufSize) {
	*buf = 0;
	int offset = _voicesOffsetsTable[num];
	if (offset != 0xFFFF) {
		const uint16 *p = _voicesOffsetsTable + offset / 2;
		offset = (*p++) * 2048;
		int count = *p++;
		if (segment < count) {
			File f;
			if (f.open("VOICE.VCE", _dataPath, "rb")) {
				int voiceSize = p[segment] * 2048 / 5;
				free(_voiceBuf);
				_voiceBuf = (uint8 *)malloc(voiceSize);
				if (_voiceBuf) {
					uint8 *dst = _voiceBuf;
					offset += 0x2000;
					for (int s = 0; s < count; ++s) {
						int len = p[s] * 2048;
						for (int i = 0; i < len / (0x2000 + 2048); ++i) {
							if (s == segment) {
								f.seek(offset);
								int n = 2048;
								while (n--) {
									int v = f.readByte();
									if (v & 0x80) {
										v = -(v & 0x7F);
									}
									*dst++ = (uint8)(v & 0xFF);
								}
							}
							offset += 0x2000 + 2048;
						}
						if (s == segment) {
							break;
						}
					}
					*buf = _voiceBuf;
					*bufSize = voiceSize;
				}
			}
		}
	}
}

void Resource::load_SPL(int num, const uint8 *map) {
	char fileName[32];
	snprintf(fileName, sizeof(fileName), "level%d.SPL", num);
	File f;
	if (!f.open(fileName, _dataPath, "rb")) {
		return;
	}
	free(_sfxList);
	_numSfx = 66;
	_sfxList = (SoundFx *)calloc(_numSfx, sizeof(SoundFx));
	if (!_sfxList) {
		error("Unable to allocate SoundFx table");
	}
	int offset = 0;
	for (int i = 0; i < _numSfx; ++i) {
		const int size = f.readUint16BE(); offset += 2;
		if ((size & 0x8000) != 0) {
			continue;
		}
		assert(size != 0 && (size & 1) == 0);
		if (i != 64 && (!map || map[i] != 0)) {
			_sfxList[i].offset = offset;
			_sfxList[i].len = size;
			_sfxList[i].data = (uint8 *)malloc(size);
			assert(_sfxList[i].data);
			f.read(_sfxList[i].data, size);
		} else {
			f.seek(offset + size);
		}
		offset += size;
	}
}

