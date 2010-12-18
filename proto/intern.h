
#ifndef INTERN_H__
#define INTERN_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

typedef uint8_t uint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint32_t uint32;

inline uint16 READ_BE_UINT16(const void *ptr) {
	const uint8 *b = (const uint8 *)ptr;
	return (b[0] << 8) | b[1];
}

inline uint32 READ_BE_UINT32(const void *ptr) {
	const uint8 *b = (const uint8 *)ptr;
	return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

template<typename T>
inline void SWAP(T &a, T &b) {
	T tmp = a;
	a = b;
	b = tmp;
}

struct InitPGE {
	uint16 type;
	int16 pos_x;
	int16 pos_y;
	uint16 obj_node_number;
	uint16 life;
	int16 counter_values[4];
	uint8 object_type;
	uint8 init_room;
	uint8 room_location;
	uint8 init_flags;
	uint8 colliding_icon_num;
	uint8 icon_num;
	uint8 object_id;
	uint8 skill;
	uint8 mirror_x;
	uint8 flags;
	uint8 unk1C; // collidable, collision_data_len
	uint16 text_num;
};

struct LivePGE {
	uint16 obj_type;
	int16 pos_x;
	int16 pos_y;
	uint8 anim_seq;
	uint8 room_location;
	int16 life;
	int16 counter_value;
	uint8 collision_slot;
	uint8 next_inventory_PGE;
	uint8 current_inventory_PGE;
	uint8 unkF; // unk_inventory_PGE
	uint16 anim_number;
	uint8 flags;
	uint8 index;
	uint16 first_obj_number;
	LivePGE *next_PGE_in_room;
	InitPGE *init_PGE;
};

struct GroupPGE {
	GroupPGE *next_entry;
	uint16 index;
	uint16 group_id;
};

struct Object {
	uint16 type;
	int8 dx;
	int8 dy;
	uint16 init_obj_type;
	uint8 opcode2;
	uint8 opcode1;
	uint8 flags;
	uint8 opcode3;
	uint16 init_obj_number;
	int16 opcode_arg1;
	int16 opcode_arg2;
	int16 opcode_arg3;
};

struct ObjectNode {
	uint16 last_obj_number;
	Object *objects;
	uint16 num_objects;
};

struct ObjectOpcodeArgs {
	LivePGE *pge; // arg0
	int16 a; // arg2
	int16 b; // arg4
};

struct AnimBufferState {
	int16 x;
	int16 y;
	LivePGE *pge;
};

struct AnimBuffers {
	AnimBufferState *_states[4];
	uint8 _curPos[4];

	void addState(uint8 stateNum, int16 x, int16 y, LivePGE *pge);
};

struct CollisionSlot {
	int16 ct_pos;
	CollisionSlot *prev_slot;
	LivePGE *live_pge;
	uint16 index;
};

struct CollisionSlot2 {
	CollisionSlot2 *next_slot;
	int8 *unk2;
	uint8 data_size;
	uint8 data_buf[0x10]; // XXX check size
};

struct InventoryItem {
	uint8 icon_num;
	InitPGE *init_pge;
	LivePGE *live_pge;
};

#endif // INTERN_H__
