
#include "game.h"

const uint16_t Game::_scoreTable[] = {
	0, 200, 300, 400, 500, 800, 1000, 1200, 1500, 2000, 2200, 2500, 3000, 3200, 3500, 5000
};

static const uint8_t _monsterListLevel1[] = {
	0x22, 0, 0x23, 0, 0xFF
};

static const uint8_t _monsterListLevel2[] = {
	0x22, 0, 0x23, 0, 0x4B, 0, 0x49, 1, 0x4D, 1, 0x76, 2, 0xFF
};

static const uint8_t _monsterListLevel3[] = {
	0x76, 2, 0xFF
};

static const uint8_t _monsterListLevel4_1[] = {
	0x4D, 1, 0x76, 2, 0xFF
};

static const uint8_t _monsterListLevel4_2[] = {
	0x76, 2, 0xAC, 2, 0xD7, 3, 0xFF
};

static const uint8_t _monsterListLevel5_1[] = {
	0xB0, 3, 0xD7, 3, 0xFF
};

static const uint8_t _monsterListLevel5_2[] = {
	0xB0, 3, 0xD7, 3, 0xD8, 3, 0xFF
};

const uint8_t *Game::_monsterListLevels[] = {
	_monsterListLevel1,
	_monsterListLevel2,
	_monsterListLevel3,
	_monsterListLevel4_1,
	_monsterListLevel4_2,
	_monsterListLevel5_1,
	_monsterListLevel5_2
};

const char *Game::_monsterNames[] = {
	"junky",
	"mercenai",
	"replican",
	"glue"
};

const uint16_t Game::_cutsceneLevels[] = {
	0x00, 0x2F, 0xFFFF, 0x34, 0x39, 0x35, 0xFFFF
};

const char *Game::_levelNames[] = {
	"Titan / The Jungle",
	"Titan / New Washington",
	"Titan / Death Tower Show",
	"Earth / Surface",
	"Earth / Paradise Club",
	"Planet Morphs / Surface",
	"Planet Morphs / Inner Core"
};

const Game::pge_OpcodeProc Game::_pge_opcodeTable[] = {
	/* 0x00 */
	0,
	&Game::pge_op_isInpUp,
	&Game::pge_op_isInpBackward,
	&Game::pge_op_isInpDown,
	/* 0x04 */
	&Game::pge_op_isInpForward,
	&Game::pge_op_isInpUpMod,
	&Game::pge_op_isInpBackwardMod,
	&Game::pge_op_isInpDownMod,
	/* 0x08 */
	&Game::pge_op_isInpForwardMod,
	&Game::pge_op_isInpIdle,
	&Game::pge_op_isInpNoMod,
	&Game::pge_op_getCollision0u,
	/* 0x0C */
	&Game::pge_op_getCollision00,
	&Game::pge_op_getCollision0d,
	&Game::pge_op_getCollision1u,
	&Game::pge_op_getCollision10,
	/* 0x10 */
	&Game::pge_op_getCollision1d,
	&Game::pge_op_getCollision2u,
	&Game::pge_op_getCollision20,
	&Game::pge_op_getCollision2d,
	/* 0x14 */
	&Game::pge_op_doesNotCollide0u,
	&Game::pge_op_doesNotCollide00,
	&Game::pge_op_doesNotCollide0d,
	&Game::pge_op_doesNotCollide1u,
	/* 0x18 */
	&Game::pge_op_doesNotCollide10,
	&Game::pge_op_doesNotCollide1d,
	&Game::pge_op_doesNotCollide2u,
	&Game::pge_op_doesNotCollide20,
	/* 0x1C */
	&Game::pge_op_doesNotCollide2d,
	&Game::pge_op_collides0o0d,
	&Game::pge_op_collides2o2d,
	&Game::pge_op_collides0o0u,
	/* 0x20 */
	&Game::pge_op_collides2o2u,
	&Game::pge_op_collides2u2o,
	&Game::pge_op_isInGroup,
	&Game::pge_op_updateGroup0,
	/* 0x24 */
	&Game::pge_op_updateGroup1,
	&Game::pge_op_updateGroup2,
	&Game::pge_op_updateGroup3,
	&Game::pge_op_isPiegeDead,
	/* 0x28 */
	&Game::pge_op_collides1u2o,
	&Game::pge_op_collides1u1o,
	&Game::pge_op_collides1o1u,
	&Game::pge_o_unk0x2B,
	/* 0x2C */
	&Game::pge_o_unk0x2C,
	&Game::pge_o_unk0x2D,
	&Game::pge_op_nop,
	&Game::pge_op_pickupObject,
	/* 0x30 */
	&Game::pge_op_addItemToInventory,
	&Game::pge_op_copyPiege,
	&Game::pge_op_canUseCurrentInventoryItem,
	&Game::pge_op_removeItemFromInventory,
	/* 0x34 */
	&Game::pge_o_unk0x34,
	&Game::pge_op_isInpMod,
	&Game::pge_op_setCollisionState1,
	&Game::pge_op_setCollisionState0,
	/* 0x38 */
	&Game::pge_op_isInGroup1,
	&Game::pge_op_isInGroup2,
	&Game::pge_op_isInGroup3,
	&Game::pge_op_isInGroup4,
	/* 0x3C */
	&Game::pge_o_unk0x3C,
	&Game::pge_o_unk0x3D,
	&Game::pge_op_setPiegeCounter,
	&Game::pge_op_decPiegeCounter,
	/* 0x40 */
	&Game::pge_o_unk0x40,
	&Game::pge_op_wakeUpPiege,
	&Game::pge_op_removePiege,
	&Game::pge_op_removePiegeIfNotNear,
	/* 0x44 */
	&Game::pge_op_loadPiegeCounter,
	&Game::pge_o_unk0x45,
	&Game::pge_o_unk0x46,
	&Game::pge_o_unk0x47,
	/* 0x48 */
	&Game::pge_o_unk0x48,
	&Game::pge_o_unk0x49,
	&Game::pge_o_unk0x4A,
	&Game::pge_op_killPiege,
	/* 0x4C */
	&Game::pge_op_isInCurrentRoom,
	&Game::pge_op_isNotInCurrentRoom,
	&Game::pge_op_scrollPosY,
	&Game::pge_op_playDefaultDeathCutscene,
	/* 0x50 */
	&Game::pge_o_unk0x50,
	0,
	&Game::pge_o_unk0x52,
	&Game::pge_o_unk0x53,
	/* 0x54 */
	&Game::pge_op_isPiegeNear,
	&Game::pge_op_setLife,
	&Game::pge_op_incLife,
	&Game::pge_op_setPiegeDefaultAnim,
	/* 0x58 */
	&Game::pge_op_setLifeCounter,
	&Game::pge_op_decLifeCounter,
	&Game::pge_op_playCutscene,
	&Game::pge_op_isTempVar2Set,
	/* 0x5C */
	&Game::pge_op_playDeathCutscene,
	&Game::pge_o_unk0x5D,
	&Game::pge_o_unk0x5E,
	&Game::pge_o_unk0x5F,
	/* 0x60 */
	&Game::pge_op_findAndCopyPiege,
	&Game::pge_op_isInRandomRange,
	&Game::pge_o_unk0x62,
	&Game::pge_o_unk0x63,
	/* 0x64 */
	&Game::pge_o_unk0x64,
	&Game::pge_op_addToCredits,
	&Game::pge_op_subFromCredits,
	&Game::pge_o_unk0x67,
	/* 0x68 */
	&Game::pge_op_setCollisionState2,
	&Game::pge_op_saveState,
	&Game::pge_o_unk0x6A,
	&Game::pge_op_isInGroupSlice,
	/* 0x6C */
	&Game::pge_o_unk0x6C,
	&Game::pge_op_isCollidingObject,
	&Game::pge_o_unk0x6E,
	&Game::pge_o_unk0x6F,
	/* 0x70 */
	&Game::pge_o_unk0x70,
	&Game::pge_o_unk0x71,
	&Game::pge_o_unk0x72,
	&Game::pge_o_unk0x73,
	/* 0x74 */
	&Game::pge_op_collides4u,
	&Game::pge_op_doesNotCollide4u,
	&Game::pge_op_isBelowConrad,
	&Game::pge_op_isAboveConrad,
	/* 0x78 */
	&Game::pge_op_isNotFacingConrad,
	&Game::pge_op_isFacingConrad,
	&Game::pge_op_collides2u1u,
	&Game::pge_op_displayText,
	/* 0x7C */
	&Game::pge_o_unk0x7C,
	&Game::pge_op_playSound,
	&Game::pge_o_unk0x7E,
	&Game::pge_o_unk0x7F,
	/* 0x80 */
	&Game::pge_op_setPiegePosX,
	&Game::pge_op_setPiegePosModX,
	&Game::pge_op_changeRoom,
	&Game::pge_op_hasInventoryItem,
	/* 0x84 */
	&Game::pge_op_changeLevel,
	&Game::pge_op_shakeScreen,
	&Game::pge_o_unk0x86,
	&Game::pge_op_playSoundGroup,
	/* 0x88 */
	&Game::pge_op_adjustPos,
	0,
	&Game::pge_op_setTempVar1,
	&Game::pge_op_isTempVar1Set
};

const uint8_t Game::_pge_modKeysTable[] = {
	0x40, 0x10, 0x20
};

