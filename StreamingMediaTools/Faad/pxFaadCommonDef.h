#pragma once

//#ifdef __cplusplus
//extern "C" {
//#endif

// Macros.
/* object types for AAC */
#define MAIN       1
#define LC         2
#define SSR        3
#define LTP        4
#define HE_AAC     5
#define ER_LC     17
#define ER_LTP    19
#define LD        23
#define DRM_ER_LC 27 /* special object type for DRM */

#ifdef ANALYSIS
#define DEBUGDEC        ,unsigned char print,uint16_t var,unsigned char *dbg
#define DEBUGVAR(A,B,C) ,A,B,C
	extern uint16_t dbg_count;
#else
#define DEBUGDEC
#define DEBUGVAR(A,B,C)
#endif

/* defines if an object type can be decoded by this library or not */
static unsigned char ObjectTypesTable[32] = {
		0, /*  0 NULL */
#ifdef MAIN_DEC
		1, /*  1 AAC Main */
#else
		0, /*  1 AAC Main */
#endif
		1, /*  2 AAC LC */
#ifdef SSR_DEC
		1, /*  3 AAC SSR */
#else
		0, /*  3 AAC SSR */
#endif
#ifdef LTP_DEC
		1, /*  4 AAC LTP */
#else
		0, /*  4 AAC LTP */
#endif
#ifdef SBR_DEC
		1, /*  5 SBR */
#else
		0, /*  5 SBR */
#endif
		0, /*  6 AAC Scalable */
		0, /*  7 TwinVQ */
		0, /*  8 CELP */
		0, /*  9 HVXC */
		0, /* 10 Reserved */
		0, /* 11 Reserved */
		0, /* 12 TTSI */
		0, /* 13 Main synthetic */
		0, /* 14 Wavetable synthesis */
		0, /* 15 General MIDI */
		0, /* 16 Algorithmic Synthesis and Audio FX */

		/* MPEG-4 Version 2 */
#ifdef ERROR_RESILIENCE
		1, /* 17 ER AAC LC */
		0, /* 18 (Reserved) */
#ifdef LTP_DEC
		1, /* 19 ER AAC LTP */
#else
		0, /* 19 ER AAC LTP */
#endif
		0, /* 20 ER AAC scalable */
		0, /* 21 ER TwinVQ */
		0, /* 22 ER BSAC */
#ifdef LD_DEC
		1, /* 23 ER AAC LD */
#else
		0, /* 23 ER AAC LD */
#endif
		0, /* 24 ER CELP */
		0, /* 25 ER HVXC */
		0, /* 26 ER HILN */
		0, /* 27 ER Parametric */
#else /* No ER defined */
		0, /* 17 ER AAC LC */
		0, /* 18 (Reserved) */
		0, /* 19 ER AAC LTP */
		0, /* 20 ER AAC scalable */
		0, /* 21 ER TwinVQ */
		0, /* 22 ER BSAC */
		0, /* 23 ER AAC LD */
		0, /* 24 ER CELP */
		0, /* 25 ER HVXC */
		0, /* 26 ER HILN */
		0, /* 27 ER Parametric */
#endif
		0, /* 28 (Reserved) */
		0, /* 29 (Reserved) */
		0, /* 30 (Reserved) */
		0  /* 31 (Reserved) */
	};

#if 1
#define INLINE __inline
#else
#define INLINE inline
#endif

#if 0 //defined(_WIN32) && !defined(_WIN32_WCE)
#define ALIGN __declspec(align(16))
#else
#define ALIGN
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define ERROR_RESILIENCE


/* Allow decoding of MAIN profile AAC */
#define MAIN_DEC
/* Allow decoding of SSR profile AAC */
//#define SSR_DEC
/* Allow decoding of LTP profile AAC */
#define LTP_DEC
/* Allow decoding of LD profile AAC */
#define LD_DEC
/* Allow decoding of Digital Radio Mondiale (DRM) */
//#define DRM
//#define DRM_PS

/* LD can't do without LTP */
#ifdef LD_DEC
#ifndef ERROR_RESILIENCE
#define ERROR_RESILIENCE
#endif
#ifndef LTP_DEC
#define LTP_DEC
#endif
#endif

#define ALLOW_SMALL_FRAMELENGTH


#define SBR_DEC
//#define SBR_LOW_POWER
#define PS_DEC

#ifdef SBR_LOW_POWER
#undef PS_DEC
#endif

/* FIXED POINT: No MAIN decoding */
#ifdef FIXED_POINT
# ifdef MAIN_DEC
#  undef MAIN_DEC
# endif
#endif // FIXED_POINT

#ifdef FIXED_POINT
#define DIV_R(A, B) (((int64_t)A << REAL_BITS)/B)
#define DIV_C(A, B) (((int64_t)A << COEF_BITS)/B)
#else
#define DIV_R(A, B) ((A)/(B))
#define DIV_C(A, B) ((A)/(B))
#endif

#ifndef SBR_LOW_POWER
#define qmf_t complex_t
#define QMF_RE(A) RE(A)
#define QMF_IM(A) IM(A)
#else
#define qmf_t real_t
#define QMF_RE(A) (A)
#define QMF_IM(A)
#endif


/* END COMPILE TIME DEFINITIONS */

#if defined(_WIN32) && !defined(__MINGW32__)

#include <stdlib.h>

typedef unsigned __int64 uint64_t;
//typedef unsigned __int32 unsigned long;
typedef unsigned __int16 uint16_t;
//typedef unsigned __int8 unsigned char;
//typedef signed __int64 int64_t;
//typedef signed __int32 int32_t;
typedef signed __int16 int16_t;
//typedef signed __int8  int8_t;
//typedef float float32_t;


#else

#include <stdio.h>
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#if STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# if HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif
#if HAVE_STRING_H
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif
#if HAVE_INTTYPES_H
# include <inttypes.h>
#else
# if HAVE_STDINT_H
#  include <stdint.h>
# else
/* we need these... */
#ifndef __TCS__
typedef unsigned long long uint64_t;
typedef signed long long int64_t;
#else
typedef unsigned long uint64_t;
typedef signed long int64_t;
#endif
typedef unsigned long unsigned long;
typedef unsigned short uint16_t;
typedef unsigned char unsigned char;
typedef signed long int32_t;
typedef signed short int16_t;
//typedef signed char int8_t;
# endif
#endif
#if HAVE_UNISTD_H
//# include <unistd.h>
#endif

#ifndef HAVE_FLOAT32_T
typedef float float32_t;
#endif

#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr(), *strrchr();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy((s), (d), (n))
#  define memmove(d, s, n) bcopy((s), (d), (n))
# endif
#endif

#endif

#ifdef WORDS_BIGENDIAN
#define ARCH_IS_BIG_ENDIAN
#endif

/* FIXED_POINT doesn't work with MAIN and SSR yet */
#ifdef FIXED_POINT
  #undef MAIN_DEC
  #undef SSR_DEC
#endif


#if defined(FIXED_POINT)

  #include "fixed.h"

#elif defined(USE_DOUBLE_PRECISION)

  typedef double real_t;

  #include <math.h>

  #define MUL_R(A,B) ((A)*(B))
  #define MUL_C(A,B) ((A)*(B))
  #define MUL_F(A,B) ((A)*(B))

  /* Complex multiplication */
  static INLINE void ComplexMult(real_t *y1, real_t *y2,
      real_t x1, real_t x2, real_t c1, real_t c2)
  {
      *y1 = MUL_F(x1, c1) + MUL_F(x2, c2);
      *y2 = MUL_F(x2, c1) - MUL_F(x1, c2);
  }

  #define REAL_CONST(A) ((real_t)(A))
  #define COEF_CONST(A) ((real_t)(A))
  #define Q2_CONST(A) ((real_t)(A))
  #define FRAC_CONST(A) ((real_t)(A)) /* pure fractional part */

#else /* Normal floating point operation */

  typedef float real_t;

  #define MUL_R(A,B) ((A)*(B))
  #define MUL_C(A,B) ((A)*(B))
  #define MUL_F(A,B) ((A)*(B))

  #define REAL_CONST(A) ((real_t)(A))
  #define COEF_CONST(A) ((real_t)(A))
  #define Q2_CONST(A) ((real_t)(A))
  #define FRAC_CONST(A) ((real_t)(A)) /* pure fractional part */

  /* Complex multiplication */
  static INLINE void ComplexMult(real_t *y1, real_t *y2,
      real_t x1, real_t x2, real_t c1, real_t c2)
  {
      *y1 = MUL_F(x1, c1) + MUL_F(x2, c2);
      *y2 = MUL_F(x2, c1) - MUL_F(x1, c2);
  }


  #if defined(_WIN32) && !defined(__MINGW32__)
    #define HAS_LRINTF
    /*static INLINE int lrintf(float f)
    {
        int i;
        __asm
        {
            fld   f
            fistp i
        }
        return i;
    }*/
  #elif (defined(__i386__) && defined(__GNUC__) && \
	!defined(__CYGWIN__) && !defined(__MINGW32__))
    #ifndef HAVE_LRINTF
    #define HAS_LRINTF
    // from http://www.stereopsis.com/FPU.html
    static INLINE int lrintf(float f)
    {
        int i;
        __asm__ __volatile__ (
            "flds %1        \n\t"
            "fistpl %0      \n\t"
            : "=m" (i)
            : "m" (f));
        return i;
    }
    #endif /* HAVE_LRINTF */
  #endif


  #ifdef __ICL /* only Intel C compiler has fmath ??? */

    #include <mathf.h>

    #define sin sinf
    #define cos cosf
    #define log logf
    #define floor floorf
    #define ceil ceilf
    #define sqrt sqrtf

  #else

#ifdef HAVE_LRINTF
#  define HAS_LRINTF
#  define _ISOC9X_SOURCE 1
#  define _ISOC99_SOURCE 1
#  define __USE_ISOC9X   1
#  define __USE_ISOC99   1
#endif

    #include <math.h>

#ifdef HAVE_SINF
#  define sin sinf
#error
#endif
#ifdef HAVE_COSF
#  define cos cosf
#endif
#ifdef HAVE_LOGF
#  define log logf
#endif
#ifdef HAVE_EXPF
#  define exp expf
#endif
#ifdef HAVE_FLOORF
#  define floor floorf
#endif
#ifdef HAVE_CEILF
#  define ceil ceilf
#endif
#ifdef HAVE_SQRTF
#  define sqrt sqrtf
#endif

  #endif

#endif

#ifndef HAS_LRINTF
/* standard cast */
#define lrintf(f) ((int32_t)(f))
#endif

typedef real_t complex_t[2];
#define RE(A) A[0]
#define IM(A) A[1]


/* common functions */
unsigned char cpu_has_sse(void);
unsigned long ne_rng(unsigned long *__r1, unsigned long *__r2);
unsigned long wl_min_lzc(unsigned long x);
#ifdef FIXED_POINT
#define LOG2_MIN_INF REAL_CONST(-10000)
int32_t log2_int(unsigned long val);
int32_t log2_fix(unsigned long val);
int32_t pow2_int(real_t val);
real_t pow2_fix(real_t val);
#endif

unsigned char get_sr_index(const unsigned long samplerate);
unsigned char max_pred_sfb(const unsigned char sr_index);
unsigned char max_tns_sfb(const unsigned char sr_index, const unsigned char object_type,
                    const unsigned char is_short);
unsigned long get_sample_rate(const unsigned char sr_index);
int can_decode_ot(const unsigned char object_type);

void *faad_malloc(size_t size);
void faad_free(void *b);

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2 /* PI/2 */
#define M_PI_2 1.57079632679489661923
#endif

// add by gzl
/* First object type that has ER */
#define ER_OBJECT_START 17

// Structures.
typedef struct mp4AudioSpecificConfig
{
	/* Audio Specific Info */
	unsigned char objectTypeIndex;
	unsigned char samplingFrequencyIndex;
	unsigned long samplingFrequency;
	unsigned char channelsConfiguration;

	/* GA Specific Info */
	unsigned char frameLengthFlag;
	unsigned char dependsOnCoreCoder;
	unsigned short coreCoderDelay;
	unsigned char extensionFlag;
	unsigned char aacSectionDataResilienceFlag;
	unsigned char aacScalefactorDataResilienceFlag;
	unsigned char aacSpectralDataResilienceFlag;
	unsigned char epConfig;

	char sbr_present_flag;
	char forceUpSampling;
	char downSampledSBR;
} mp4AudioSpecificConfig;

typedef struct program_config
{
	unsigned char element_instance_tag;
	unsigned char object_type;
	unsigned char sf_index;
	unsigned char num_front_channel_elements;
	unsigned char num_side_channel_elements;
	unsigned char num_back_channel_elements;
	unsigned char num_lfe_channel_elements;
	unsigned char num_assoc_data_elements;
	unsigned char num_valid_cc_elements;
	unsigned char mono_mixdown_present;
	unsigned char mono_mixdown_element_number;
	unsigned char stereo_mixdown_present;
	unsigned char stereo_mixdown_element_number;
	unsigned char matrix_mixdown_idx_present;
	unsigned char pseudo_surround_enable;
	unsigned char matrix_mixdown_idx;
	unsigned char front_element_is_cpe[16];
	unsigned char front_element_tag_select[16];
	unsigned char side_element_is_cpe[16];
	unsigned char side_element_tag_select[16];
	unsigned char back_element_is_cpe[16];
	unsigned char back_element_tag_select[16];
	unsigned char lfe_element_tag_select[16];
	unsigned char assoc_data_element_tag_select[16];
	unsigned char cc_element_is_ind_sw[16];
	unsigned char valid_cc_element_tag_select[16];

	unsigned char channels;

	unsigned char comment_field_bytes;
	unsigned char comment_field_data[257];

	/* extra added values */
	unsigned char num_front_channels;
	unsigned char num_side_channels;
	unsigned char num_back_channels;
	unsigned char num_lfe_channels;
	unsigned char sce_channel[16];
	unsigned char cpe_channel[16];
}program_config;

typedef struct _bitfile
{
	/* bit input */
	unsigned long bufa;
	unsigned long bufb;
	unsigned long bits_left;
	unsigned long buffer_size; /* size of the buffer in bytes */
	unsigned long bytes_left;
	unsigned char error;
	unsigned long *tail;
	unsigned long *start;
	const void *buffer;
} bitfile;


//#ifdef __cplusplus
//}
//#endif
