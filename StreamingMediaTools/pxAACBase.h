#ifndef _PX_AAC_BASE_H
#define _PX_AAC_BASE_H

typedef unsigned char uint8_t;
//typedef unsigned short int uint16_t;
typedef unsigned __int32 uint32_t;
//typedef unsigned long long int uint64_t;
//typedef char int8_t;
//typedef short int int16_t;
//typedef __int32 int32_t;
//typedef long long int int64_t;

typedef struct PutBitContext {
	uint32_t bit_buf;
	int bit_left;
	uint8_t *buf, *buf_ptr, *buf_end;
	int size_in_bits;
} PutBitContext;

int adts_write_frame_header(uint8_t *buf, 
	int size, 
	int pce_size,
	char chprofile, 
	char chsampleindex, 
	char channels);

static int sampleFrequency[] = 
{
	96000,
	88200,
	64000,
	48000,
	44100,
	32000, // 5
	24000,
	22050,
	16000,
	12000,
	11025,
	8000,
	7350
};

#endif

