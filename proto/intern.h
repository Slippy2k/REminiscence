
#ifndef INTERN_H__
#define INTERN_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#ifndef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))
#endif
#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

inline uint16_t READ_BE_UINT16(const void *ptr) {
	const uint8_t *b = (const uint8_t *)ptr;
	return (b[0] << 8) | b[1];
}

inline uint32_t READ_BE_UINT32(const void *ptr) {
	const uint8_t *b = (const uint8_t *)ptr;
	return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

inline uint16_t READ_LE_UINT16(const void *ptr) {
	const uint8_t *b = (const uint8_t *)ptr;
	return (b[1] << 8) | b[0];
}

inline uint32_t READ_LE_UINT32(const void *ptr) {
	const uint8_t *b = (const uint8_t *)ptr;
	return (b[3] << 24) | (b[2] << 16) | (b[1] << 8) | b[0];
}

template<typename T>
inline void SWAP(T &a, T &b) {
	T tmp = a;
	a = b;
	b = tmp;
}

struct InitPGE {
	uint16_t type;
	int16_t pos_x;
	int16_t pos_y;
	uint16_t obj_node_number;
	uint16_t life;
	int16_t counter_values[4];
	uint8_t object_type;
	uint8_t init_room;
	uint8_t room_location;
	uint8_t init_flags;
	uint8_t colliding_icon_num;
	uint8_t icon_num;
	uint8_t object_id;
	uint8_t skill;
	uint8_t mirror_x;
	uint8_t flags;
	uint8_t unk1C; // collidable, collision_data_len
	uint16_t text_num;
};

struct LivePGE {
	uint16_t obj_type;
	int16_t pos_x;
	int16_t pos_y;
	uint8_t anim_seq;
	uint8_t room_location;
	int16_t life;
	int16_t counter_value;
	uint8_t collision_slot;
	uint8_t next_inventory_PGE;
	uint8_t current_inventory_PGE;
	uint8_t unkF; // unk_inventory_PGE
	uint16_t anim_number;
	uint8_t flags;
	uint8_t index;
	uint16_t first_obj_number;
	LivePGE *next_PGE_in_room;
	InitPGE *init_PGE;
};

struct GroupPGE {
	GroupPGE *next_entry;
	uint16_t index;
	uint16_t group_id;
};

struct Object {
	uint16_t type;
	int8_t dx;
	int8_t dy;
	uint16_t init_obj_type;
	uint8_t opcode2;
	uint8_t opcode1;
	uint8_t flags;
	uint8_t opcode3;
	uint16_t init_obj_number;
	int16_t opcode_arg1;
	int16_t opcode_arg2;
	int16_t opcode_arg3;
};

struct ObjectNode {
	uint16_t last_obj_number;
	Object *objects;
	uint16_t num_objects;
};

struct ObjectOpcodeArgs {
	LivePGE *pge; // arg0
	int16_t a; // arg2
	int16_t b; // arg4
};

struct AnimBufferState {
	int16_t x;
	int16_t y;
	LivePGE *pge;
};

struct AnimBuffers {
	AnimBufferState *_states[4];
	uint8_t _curPos[4];

	void addState(uint8_t stateNum, int16_t x, int16_t y, LivePGE *pge);
};

struct CollisionSlot {
	int16_t ct_pos;
	CollisionSlot *prev_slot;
	LivePGE *live_pge;
	uint16_t index;
};

struct CollisionSlot2 {
	CollisionSlot2 *next_slot;
	int8_t *unk2;
	uint8_t data_size;
	uint8_t data_buf[0x10]; // XXX check size
};

struct InventoryItem {
	uint8_t icon_num;
	InitPGE *init_pge;
	LivePGE *live_pge;
};

struct PlayerInput {
	enum {
		kDirectionUp    = 1 << 0,
		kDirectionDown  = 1 << 1,
		kDirectionLeft  = 1 << 2,
		kDirectionRight = 1 << 3
	};
	uint8_t dirMask;
	bool enter;
	bool space;
	bool shift;
	bool backspace;
	bool quit;
	enum {
		kTouchNone,
		kTouchUp,
		kTouchDown
	};
	struct {
		int x, y;
		int press;
	} touch;
};

#endif // INTERN_H__
