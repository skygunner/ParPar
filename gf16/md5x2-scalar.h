#include <string.h> // memcpy+memset
#include "platform.h"

#if defined(__GNUC__) && defined(PLATFORM_AMD64)
# define MD5_USE_ASM
# include "md5x2-x86-asm.h"
#endif

#include "md5-scalar-base.h"

#define _FN(f) f##_scalar
#define MD5X2

#include "md5x2-base.h"

#ifdef MD5X2
# undef MD5X2
#endif

#undef _FN
#undef ROTATE
#undef ADD
#undef VAL
#undef word_t
#undef INPUT
#undef LOAD

#undef F
#undef G
#undef H
#undef I
#undef ADDF

#ifdef MD5_USE_ASM
# undef MD5_USE_ASM
#endif

static HEDLEY_ALWAYS_INLINE void md5_extract_x2_scalar(void* dst, void* state, const int idx) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint32_t* state_ = (uint32_t*)state + idx*4;
	uint32_t* dst_ = (uint32_t*)dst;
	for(int i=0; i<4; i++)
		dst_[i] = BSWAP(state_[i]);
#else
	memcpy(dst, (uint32_t*)state + idx*4, 16);
#endif
}
static HEDLEY_ALWAYS_INLINE void md5_init_lane_x2_scalar(void* state, const int idx) {
	uint32_t* state_ = (uint32_t*)state;
	state_[0 + idx*4] = 0x67452301L;
	state_[1 + idx*4] = 0xefcdab89L;
	state_[2 + idx*4] = 0x98badcfeL;
	state_[3 + idx*4] = 0x10325476L;
}
