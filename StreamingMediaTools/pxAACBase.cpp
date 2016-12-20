#include "pxAACBase.h"

#ifndef AV_WL32
#   define AV_WL32(p, darg) do {                \
	unsigned d = (darg);                    \
	((uint8_t*)(p))[0] = (d);               \
	((uint8_t*)(p))[1] = (d)>>8;            \
	((uint8_t*)(p))[2] = (d)>>16;           \
	((uint8_t*)(p))[3] = (d)>>24;           \
	} while(0)
#endif

#ifndef AV_WB32
#   define AV_WB32(p, darg) do {                \
	unsigned d = (darg);                    \
	((uint8_t*)(p))[3] = (d);               \
	((uint8_t*)(p))[2] = (d)>>8;            \
	((uint8_t*)(p))[1] = (d)>>16;           \
	((uint8_t*)(p))[0] = (d)>>24;           \
	} while(0)
#endif

static inline void init_put_bits(PutBitContext *s, uint8_t *buffer,
	int buffer_size)
{
	if (buffer_size < 0) 
	{
		buffer_size = 0;
		buffer      = 0;
	}

	s->size_in_bits = 8 * buffer_size;
	s->buf          = buffer;
	s->buf_end      = s->buf + buffer_size;
	s->buf_ptr      = s->buf;
	s->bit_left     = 32;
	s->bit_buf      = 0;
}

/**
 * Write up to 31 bits into a bitstream.
 * Use put_bits32 to write 32 bits.
 */
static inline void put_bits(PutBitContext *s, int n, unsigned int value)
{
    unsigned int bit_buf;
    int bit_left;

    //av_assert2(n <= 31 && value < (1U << n));

    bit_buf  = s->bit_buf;
    bit_left = s->bit_left;

    /* XXX: optimize */
#ifdef BITSTREAM_WRITER_LE
    bit_buf |= value << (32 - bit_left);
    if (n >= bit_left) {
        //av_assert2(s->buf_ptr+3<s->buf_end);
        AV_WL32(s->buf_ptr, bit_buf);
        s->buf_ptr += 4;
        bit_buf     = value >> bit_left;
        bit_left   += 32;
    }
    bit_left -= n;
#else
    if (n < bit_left) {
        bit_buf     = (bit_buf << n) | value;
        bit_left   -= n;
    } else {
        bit_buf   <<= bit_left;
        bit_buf    |= value >> (n - bit_left);
        //av_assert2(s->buf_ptr+3<s->buf_end);
        AV_WB32(s->buf_ptr, bit_buf);
        s->buf_ptr += 4;
        bit_left   += 32 - n;
        bit_buf     = value;
    }
#endif

    s->bit_buf  = bit_buf;
    s->bit_left = bit_left;
}

/**
 * Pad the end of the output stream with zeros.
 */
static inline void flush_put_bits(PutBitContext *s)
{
#ifndef BITSTREAM_WRITER_LE
    if (s->bit_left < 32)
        s->bit_buf <<= s->bit_left;
#endif
    while (s->bit_left < 32) {
        /* XXX: should test end of buffer */
#ifdef BITSTREAM_WRITER_LE
        *s->buf_ptr++ = s->bit_buf;
        s->bit_buf  >>= 8;
#else
        *s->buf_ptr++ = s->bit_buf >> 24;
        s->bit_buf  <<= 8;
#endif
        s->bit_left  += 8;
    }
    s->bit_left = 32;
    s->bit_buf  = 0;
}

#define  ADTS_HEADER_SIZE (7)
#define ADTS_MAX_FRAME_BYTES ((1 << 13) - 1)
//static const int samplerate_table[] =
//{ 44100, 22050, 11025, 96000, 48000, 32000, 24000, 16000, 8000 };

int adts_write_frame_header(uint8_t *buf, 
							int size, 
							int pce_size,
							char chprofile, 
							char chsampleindex, 
							char channels)
{
	PutBitContext pb;
	pce_size = 0;

	unsigned full_frame_size = (unsigned)ADTS_HEADER_SIZE + size + pce_size;
	if (full_frame_size > ADTS_MAX_FRAME_BYTES) {
		//av_log(NULL, AV_LOG_ERROR, "ADTS frame size too large: %u (max %d)\n",
		//	full_frame_size, ADTS_MAX_FRAME_BYTES);
		return -1; //AVERROR_INVALIDDATA;
	}

	init_put_bits(&pb, buf, ADTS_HEADER_SIZE);

	/* adts_fixed_header */
	put_bits(&pb, 12, 0xfff);   /* syncword */
	put_bits(&pb, 1, 0);        /* ID */
	put_bits(&pb, 2, 0);        /* layer */
	put_bits(&pb, 1, 1);        /* protection_absent */
	put_bits(&pb, 2, chprofile);		/* profile_objecttype */
	put_bits(&pb, 4, chsampleindex);		//32000
	put_bits(&pb, 1, 0);        /* private_bit */
	put_bits(&pb, 3, channels);		/* channel_configuration */
	put_bits(&pb, 1, 0);        /* original_copy */
	put_bits(&pb, 1, 0);        /* home */

	/* adts_variable_header */
	put_bits(&pb, 1, 0);        /* copyright_identification_bit */
	put_bits(&pb, 1, 0);        /* copyright_identification_start */
	put_bits(&pb, 13, full_frame_size); /* aac_frame_length */
	put_bits(&pb, 11, 0x7ff);   /* adts_buffer_fullness */
	put_bits(&pb, 2, 0);        /* number_of_raw_data_blocks_in_frame */

	flush_put_bits(&pb);

	return 0;
}