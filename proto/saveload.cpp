
#include <sys/param.h>
#include "file.h"
#include "game.h"

static const uint32_t kPtr0 = 0xFFFFFFFF;

void Game::saveState() {
	char path[MAXPATHLEN];
	snprintf(path, sizeof(path), "fb%d.state.z", _currentLevel);
	File f(true);
	if (!f.open(path, "wb")) {
		return;
	}
	f.writeByte(_skillLevel);
	f.writeUint32BE(_score);
	f.writeUint32BE((_col_slots2Cur == 0) ? kPtr0 : _col_slots2Cur - &_col_slots2[0]);
	f.writeUint32BE((_col_slots2Next == 0) ? kPtr0 : _col_slots2Next - &_col_slots2[0]);
	for (int i = 0; i < _res._pgeNum; ++i) {
		LivePGE *pge = &_pgeLive[i];
		f.writeUint16BE(pge->obj_type);
		f.writeUint16BE(pge->pos_x);
		f.writeUint16BE(pge->pos_y);
		f.writeByte(pge->anim_seq);
		f.writeByte(pge->room_location);
		f.writeUint16BE(pge->life);
		f.writeUint16BE(pge->counter_value);
		f.writeByte(pge->collision_slot);
		f.writeByte(pge->next_inventory_PGE);
		f.writeByte(pge->current_inventory_PGE);
		f.writeByte(pge->unkF);
		f.writeUint16BE(pge->anim_number);
		f.writeByte(pge->flags);
		f.writeByte(pge->index);
		f.writeUint16BE(pge->first_obj_number);
		f.writeUint32BE((pge->next_PGE_in_room == 0) ? kPtr0 : pge->next_PGE_in_room - &_pgeLive[0]);
		f.writeUint32BE((pge->init_PGE == 0) ? kPtr0 : pge->init_PGE - &_res._pgeInit[0]);
	}
	f.write(&_res._ctData[0x100], 0x1C00);
	for (CollisionSlot2 *cs2 = &_col_slots2[0]; cs2 < _col_slots2Cur; ++cs2) {
		f.writeUint32BE((cs2->next_slot == 0) ? kPtr0 : cs2->next_slot - &_col_slots2[0]);
		f.writeUint32BE((cs2->unk2 == 0) ? kPtr0 : cs2->unk2 - &_res._ctData[0x100]);
		f.writeByte(cs2->data_size);
		f.write(cs2->data_buf, 0x10);
	}
}

void Game::loadState() {
	char path[MAXPATHLEN];
	snprintf(path, sizeof(path), "fb%d.state.z", _currentLevel);
	File f(true);
	if (!f.open(path, "rb")) {
		return;
	}
	_skillLevel = f.readByte();
	_score = f.readUint32BE();
	memset(_pge_liveTable2, 0, sizeof(_pge_liveTable2));
	memset(_pge_liveTable1, 0, sizeof(_pge_liveTable1));
	uint32_t off = f.readUint32BE();
	_col_slots2Cur = (off == kPtr0) ? 0 : &_col_slots2[0] + off;
	off = f.readUint32BE();
	_col_slots2Next = (off == kPtr0) ? 0 : &_col_slots2[0] + off;
	for (int i = 0; i < _res._pgeNum; ++i) {
		LivePGE *pge = &_pgeLive[i];
		pge->obj_type = f.readUint16BE();
		pge->pos_x = f.readUint16BE();
		pge->pos_y = f.readUint16BE();
		pge->anim_seq = f.readByte();
		pge->room_location = f.readByte();
		pge->life = f.readUint16BE();
		pge->counter_value = f.readUint16BE();
		pge->collision_slot = f.readByte();
		pge->next_inventory_PGE = f.readByte();
		pge->current_inventory_PGE = f.readByte();
		pge->unkF = f.readByte();
		pge->anim_number = f.readUint16BE();
		pge->flags = f.readByte();
		pge->index = f.readByte();
		pge->first_obj_number = f.readUint16BE();
		off = f.readUint32BE();
		pge->next_PGE_in_room = (off == kPtr0) ? 0 : &_pgeLive[0] + off;
		off = f.readUint32BE();
		pge->init_PGE = (off == kPtr0) ? 0 : &_res._pgeInit[0] + off;
	}
	f.read(&_res._ctData[0x100], 0x1C00);
	for (CollisionSlot2 *cs2 = &_col_slots2[0]; cs2 < _col_slots2Cur; ++cs2) {
		off = f.readUint32BE();
		cs2->next_slot = (off == kPtr0) ? 0 : &_col_slots2[0] + off;
		off = f.readUint32BE();
		cs2->unk2 = (off == kPtr0) ? 0 : &_res._ctData[0x100] + off;
		cs2->data_size = f.readByte();
		f.read(cs2->data_buf, 0x10);
	}
	for (int i = 0; i < _res._pgeNum; ++i) {
		if (_res._pgeInit[i].skill <= _skillLevel) {
			LivePGE *pge = &_pgeLive[i];
			if (pge->flags & 4) {
				_pge_liveTable2[pge->index] = pge;
			}
			pge->next_PGE_in_room = _pge_liveTable1[pge->room_location];
			_pge_liveTable1[pge->room_location] = pge;
		}
	}
}

