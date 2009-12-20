
#ifndef __SEQ_H__
#define __SEQ_H__

#include "intern.h"

struct SystemStub;

struct Seq {
	enum {
		VIDEO_W    = 256,
		VIDEO_H    = 128,
		SYNC_DELAY = 150
	};

	static const char *_seqPlayerTitle;
	
	Seq(SystemStub *stub, const char *fileName);
	~Seq();
	
	void start();
	void ply_mainLoop();
	void ply_openSeqFile();
	void ply_closeSeqFile();
	void ply_readFromSeqFile(uint8 *buf, size_t size);
	void ply_initOffsets(const uint8 *dataPtr);	
	void ply_copyVideoBlock(const uint8 *bufData, uint8 bufNum, uint16 bufSize);
	void ply_readProcessData();
	void ply_initProcessData();
	void ply_copyProcessData();
	void ply_decodeBlit();
	void ply_processFrame();
	void ply_processFrameOp2(const uint8 *&src, uint8 *dst);
	void ply_processFrameOp3(const uint8 *&src, uint8 *dst);
	void ply_processFrameOp4(const uint8 *&src, uint8 *dst);
	void ply_unpackFrame(const uint8 *&src);
	
	void vid_clearTempFrame();
	void vid_setPalSync(const uint8 *palData);
	void vid_clearPal();
	void vid_setPalBuf1(const uint8 *palData);
	
	void snd_readNextFrame();
	void snd_audioCallback(uint8 *stream, int len);
	
	SystemStub *_stub;
	
	const char *_ply_fileName;
	FILE *_ply_fileHandle;
	FILE *_ply_fileHandleSnd;
	uint32 _ply_lastTimeStamp;
	bool _ply_quitFlag;
	uint8 *_ply_ptrsTable[100];
	uint16 _ply_offTable[100];
	uint8 _ply_readBuffer[0x1800];
	uint8 _ply_processOpBuffer[0x200];
	uint8 _ply_unpackedFrameBuffer[0x400];
	const uint8 *_ply_vidStartOffset;
	const uint8 *_ply_curProcessBuffer;
	uint8 _ply_blockNum;
	uint8 *_ply_processBuffer;
	uint16 _ply_processBlockNum;	
	uint8 *_ply_tempBuffer;
	int _ply_sndFrameNum;
	
	uint8 _vid_pal1[0x300];
	uint8 _vid_tempFrame[256 * 128];
	
	uint8 _snd_buf[0x372 * 2];
	int _snd_len, _snd_pos;
};

#endif
