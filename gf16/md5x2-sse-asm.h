#include "platform.h"

#ifndef STR
# define STR_HELPER(x) #x
# define STR(x) STR_HELPER(x)
#endif

ALIGN_TO(16, static const uint32_t md5_constants[64]) = {
	// F
	0xd76aa478L, 0xe8c7b756L, 0x242070dbL, 0xc1bdceeeL, 0xf57c0fafL, 0x4787c62aL, 0xa8304613L, 0xfd469501L,
	0x698098d8L, 0x8b44f7afL, 0xffff5bb1L, 0x895cd7beL, 0x6b901122L, 0xfd987193L, 0xa679438eL, 0x49b40821L,
	
	// G (sequenced: 3,0,1,2)
	0xe9b6c7aaL, 0xf61e2562L, 0xc040b340L, 0x265e5a51L,
	0xe7d3fbc8L, 0xd62f105dL, 0x02441453L, 0xd8a1e681L,
	0x455a14edL, 0x21e1cde6L, 0xc33707d6L, 0xf4d50d87L,
	0x8d2a4c8aL, 0xa9e3e905L, 0xfcefa3f8L, 0x676f02d9L,
	
	// H (sequenced: 1,0,3,2)
	0x8771f681L, 0xfffa3942L, 0xfde5380cL, 0x6d9d6122L,
	0x4bdecfa9L, 0xa4beea44L, 0xbebfbc70L, 0xf6bb4b60L,
	0xeaa127faL, 0x289b7ec6L, 0x04881d05L, 0xd4ef3085L,
	0xe6db99e5L, 0xd9d4d039L, 0xc4ac5665L, 0x1fa27cf8L,
	
	// I
	0xf4292244L, 0x432aff97L, 0xab9423a7L, 0xfc93a039L,
	0x655b59c3L, 0x8f0ccc92L, 0xffeff47dL, 0x85845dd1L,
	0x6fa87e4fL, 0xfe2ce6e0L, 0xa3014314L, 0x4e0811a1L,
	0xf7537e82L, 0xbd3af235L, 0x2ad7d2bbL, 0xeb86d391L
};


#ifdef PLATFORM_AMD64
#define ASM_PARAMS_F(n, c0, c1) \
	[A]"+x"(A), [B]"+x"(B), [C]"+x"(C), [D]"+x"(D), [TMPI1]"=x"(tmpI1), [TMPI2]"=x"(tmpI2), [TMPF1]"=x"(tmpF1), [TMPF2]"=x"(tmpF2), \
	[cache0]"=x"(cache##c0), [cache1]"=x"(cache##c1) \
	: \
	[k]"m"(md5_constants[n]), [i0]"m"(_in[0][n/4]), [i1]"m"(_in[1][n/4]) :

#define ASM_PARAMS(n) \
	[A]"+x"(A), [B]"+x"(B), [C]"+x"(C), [D]"+x"(D), [TMPI1]"=x"(tmpI1), [TMPI2]"=x"(tmpI2), [TMPF1]"=x"(tmpF1), [TMPF2]"=x"(tmpF2) \
	: [input0]"x"(cache0), [input1]"x"(cache1), [input2]"x"(cache2), [input3]"x"(cache3), [input4]"x"(cache4), [input5]"x"(cache5), [input6]"x"(cache6), [input7]"x"(cache7), \
	[k0]"m"(md5_constants[n+0]), [k4]"m"(md5_constants[n+4]), [k8]"m"(md5_constants[n+8]), [k12]"m"(md5_constants[n+12]) :

#define FN_VARS \
	UNUSED(offset); \
	__m128i A = state[0]; \
	__m128i B = state[1]; \
	__m128i C = state[2]; \
	__m128i D = state[3]; \
	__m128i tmpI1, tmpI2, tmpF1, tmpF2; \
	const __m128i* const* HEDLEY_RESTRICT _in = (const __m128i* const* HEDLEY_RESTRICT)data; \
	__m128i cache0, cache1, cache2, cache3, cache4, cache5, cache6, cache7; \
	asm("" : "=x"(tmpI1), "=x"(tmpI2), "=x"(tmpF1), "=x"(tmpF2) ::)

#else
#define ASM_PARAMS_F(n, c0, c1) \
	[A]"+x"(A), [B]"+x"(B), [C]"+x"(C), [D]"+x"(D), [TMPI1]"=x"(tmpI1), [TMPI2]"=x"(tmpI2), [TMPF1]"=x"(tmpF1), [TMPF2]"=x"(tmpF2) \
	: \
	[k]"m"(md5_constants[n]), [i0]"m"(_in[0][n/4]), [i1]"m"(_in[1][n/4]), [scratch0]"m"(scratch[c0*4]), [scratch1]"m"(scratch[c1*4]) :
	
#define ASM_PARAMS(n) \
	[A]"+x"(A), [B]"+x"(B), [C]"+x"(C), [D]"+x"(D), [TMPI1]"=x"(tmpI1), [TMPI2]"=x"(tmpI2), [TMPF1]"=x"(tmpF1), [TMPF2]"=x"(tmpF2) \
	: \
	[k0]"m"(md5_constants[n+0]), [k4]"m"(md5_constants[n+4]), [k8]"m"(md5_constants[n+8]), [k12]"m"(md5_constants[n+12]), \
	[input0]"m"(scratch[0]), [input1]"m"(scratch[4]), [input2]"m"(scratch[8]), [input3]"m"(scratch[12]), [input4]"m"(scratch[16]), [input5]"m"(scratch[20]), [input6]"m"(scratch[24]), [input7]"m"(scratch[28]) :
	
#define FN_VARS \
	UNUSED(offset); \
	__m128i A = state[0]; \
	__m128i B = state[1]; \
	__m128i C = state[2]; \
	__m128i D = state[3]; \
	__m128i tmpI1, tmpI2, tmpF1, tmpF2; \
	const __m128i* const* HEDLEY_RESTRICT _in = (const __m128i* const* HEDLEY_RESTRICT)data; \
	ALIGN_TO(16, uint32_t scratch[32]); \
	asm("" : "=x"(tmpI1), "=x"(tmpI2), "=x"(tmpF1), "=x"(tmpF2) ::)

#endif


#ifdef __SSE2__
static HEDLEY_ALWAYS_INLINE void md5_process_block_x2_sse(__m128i* state, const char* const* HEDLEY_RESTRICT data, size_t offset) {
	FN_VARS;
	
#define LOADK(offs) \
	"movdqa %[k" offs "], %[TMPI2]\n" \
	"pshufd $0b01000100, %[TMPI2], %[TMPI1]\n" \
	"punpckhqdq %[TMPI2], %[TMPI2]\n"
#define ROUND_X(A, B, I, R) \
	"paddd " I ", %[" STR(A) "]\n" \
	"paddd %[TMPF1], %[" STR(A) "]\n" \
	"pshufd $0b10100000, %[" STR(A) "], %[" STR(A) "]\n" \
	"psrlq $" STR(R) ", %[" STR(A) "]\n" \
	"paddd %[" STR(B) "], %[" STR(A) "]\n"

#ifdef PLATFORM_AMD64
#define READ4 \
	"movdqu %[i0], %[cache0]\n" \
	"movdqu %[i1], %[TMPI1]\n" \
	"movdqa %[cache0], %[cache1]\n" \
	"punpcklqdq %[TMPI1], %[cache0]\n" \
	"punpckhqdq %[TMPI1], %[cache1]\n" \
	LOADK("") \
	"paddd %[cache0], %[TMPI1]\n" \
	"paddd %[cache1], %[TMPI2]\n"
#else
#define READ4 \
	"movdqu %[i0], %[TMPF1]\n" \
	"movdqu %[i1], %[TMPI2]\n" \
	"movdqa %[TMPF1], %[TMPF2]\n" \
	"punpcklqdq %[TMPI2], %[TMPF1]\n" \
	"punpckhqdq %[TMPI2], %[TMPF2]\n" \
	"movaps %[TMPF1], %[scratch0]\n" \
	"movaps %[TMPF2], %[scratch1]\n" \
	LOADK("") \
	"paddd %[TMPF1], %[TMPI1]\n" \
	"paddd %[TMPF2], %[TMPI2]\n"
#endif

#define ROUND_F(A, B, C, D, I, R) \
	"movdqa %[" STR(D) "], %[TMPF1]\n" \
	"pxor %[" STR(C) "], %[TMPF1]\n" \
	"pand %[" STR(B) "], %[TMPF1]\n" \
	"pxor %[" STR(D) "], %[TMPF1]\n" \
	ROUND_X(A, B, I, R)
#define ROUND_H(A, B, C, D, I, R) \
	"movdqa %[" STR(D) "], %[TMPF1]\n" \
	"pxor %[" STR(C) "], %[TMPF1]\n" \
	"pxor %[" STR(B) "], %[TMPF1]\n" \
	ROUND_X(A, B, I, R)
#define ROUND_I(A, B, C, D, I, R) \
	"movdqa %[" STR(D) "], %[TMPF1]\n" \
	"pxor %[TMPF2], %[TMPF1]\n" \
	"por %[" STR(B) "], %[TMPF1]\n" \
	"pxor %[" STR(C) "], %[TMPF1]\n" \
	ROUND_X(A, B, I, R)

#define ROUND_G(A, B, C, D, I, R) \
	"movdqa %[" STR(D) "], %[TMPF1]\n" \
	"paddd " I ", %[" STR(A) "]\n" \
	"pandn %[" STR(C) "], %[TMPF1]\n" \
	"movdqa %[" STR(D) "], %[TMPF2]\n" \
	"paddd %[TMPF1], %[" STR(A) "]\n" \
	"pand %[" STR(B) "], %[TMPF2]\n" \
	"paddd %[TMPF2], %[" STR(A) "]\n" \
	"pshufd $0b10100000, %[" STR(A) "], %[" STR(A) "]\n" \
	"psrlq $" STR(R) ", %[" STR(A) "]\n" \
	"paddd %[" STR(B) "], %[" STR(A) "]\n"

#define RF4(offs, r1, r2) asm( \
	READ4 \
	ROUND_F(A, B, C, D, "%[TMPI1]", 25) \
	"psrlq $32, %[TMPI1]\n" \
	ROUND_F(D, A, B, C, "%[TMPI1]", 20) \
	ROUND_F(C, D, A, B, "%[TMPI2]", 15) \
	"psrlq $32, %[TMPI2]\n" \
	ROUND_F(B, C, D, A, "%[TMPI2]", 10) \
: ASM_PARAMS_F(offs, r1, r2));
	
#define RG4(offs, rs, r1, r2) \
	"movaps %[input" STR(r1) "], %[TMPF2]\n" \
	"shufps $0b11011000, %[input" STR(r2) "], %[TMPF2]\n" \
	LOADK(STR(offs)) \
	"shufps $0b11011000, %[TMPF2], %[TMPF2]\n" \
	"paddd %[input" STR(rs) "], %[TMPI1]\n" \
	"paddd %[TMPF2], %[TMPI2]\n" \
	"pshufd $0b10110001, %[TMPI1], %[TMPF2]\n" \
	\
	ROUND_G(A, B, C, D, "%[TMPF2]", 27) \
	ROUND_G(D, A, B, C, "%[TMPI2]", 23) \
	"psrlq $32, %[TMPI2]\n" \
	ROUND_G(C, D, A, B, "%[TMPI2]", 18) \
	ROUND_G(B, C, D, A, "%[TMPI1]", 12)
	
#define RH4(offs, r1, r2, r3, r4) \
	"movaps %[input" STR(r1) "], %[TMPF1]\n" \
	"shufps $0b10001101, %[input" STR(r2) "], %[TMPF1]\n" \
	"movaps %[input" STR(r3) "], %[TMPF2]\n" \
	"shufps $0b01110010, %[TMPF1], %[TMPF1]\n" \
	"shufps $0b10001101, %[input" STR(r4) "], %[TMPF2]\n" \
	LOADK(STR(offs)) \
	"shufps $0b01110010, %[TMPF2], %[TMPF2]\n" \
	"paddd %[TMPF1], %[TMPI1]\n" \
	"paddd %[TMPF2], %[TMPI2]\n" \
	\
	"pshufd $0b11110101, %[TMPI1], %[TMPF2]\n" \
	ROUND_H(A, B, C, D, "%[TMPF2]", 28) \
	ROUND_H(D, A, B, C, "%[TMPI1]", 21) \
	"pshufd $0b11110101, %[TMPI2], %[TMPF2]\n" \
	ROUND_H(C, D, A, B, "%[TMPF2]", 16) \
	ROUND_H(B, C, D, A, "%[TMPI2]",  9)
	
#define RI4(offs, r1, r2, r3, r4) \
	"movaps %[input" STR(r1) "], %[TMPF1]\n" \
	"shufps $0b11011000, %[input" STR(r2) "], %[TMPF1]\n" \
	LOADK(STR(offs)) \
	"shufps $0b11011000, %[TMPF1], %[TMPF1]\n" \
	"paddd %[TMPF1], %[TMPI1]\n" \
	"movaps %[input" STR(r3) "], %[TMPF1]\n" \
	"shufps $0b11011000, %[input" STR(r4) "], %[TMPF1]\n" \
	"shufps $0b11011000, %[TMPF1], %[TMPF1]\n" \
	"paddd %[TMPF1], %[TMPI2]\n" \
	\
	ROUND_I(A, B, C, D, "%[TMPI1]", 26) \
	"psrlq $32, %[TMPI1]\n" \
	ROUND_I(D, A, B, C, "%[TMPI1]", 22) \
	ROUND_I(C, D, A, B, "%[TMPI2]", 17) \
	"psrlq $32, %[TMPI2]\n" \
	ROUND_I(B, C, D, A, "%[TMPI2]", 11)
	
	RF4(0, 0, 1)
	RF4(4, 2, 3)
	RF4(8, 4, 5)
	RF4(12, 6, 7)
	
	asm(
		RG4(0, 0, 3, 5)
		RG4(4, 2, 5, 7)
		RG4(8, 4, 7, 1)
		RG4(12, 6, 1, 3)
	: ASM_PARAMS(16));
	
	asm(
		RH4(0, 2, 4, 5, 7)
		RH4(4, 0, 2, 3, 5)
		RH4(8, 6, 0, 1, 3)
		RH4(12, 4, 6, 7, 1)
	: ASM_PARAMS(32));
	
	asm(
		"pcmpeqb %[TMPF2], %[TMPF2]\n"
		RI4(0, 0, 3, 7, 2)
		RI4(4, 6, 1, 5, 0)
		RI4(8, 4, 7, 3, 6)
		RI4(12, 2, 5, 1, 4)
	: ASM_PARAMS(48));
	
	asm("" :: "x"(tmpI1), "x"(tmpI2), "x"(tmpF1), "x"(tmpF2) :); // for some reason, the above can fail without this
	
	state[0] = _mm_add_epi32(A, state[0]);
	state[1] = _mm_add_epi32(B, state[1]);
	state[2] = _mm_add_epi32(C, state[2]);
	state[3] = _mm_add_epi32(D, state[3]);
#undef LOADK
#undef ROUND_X
#undef READ4
#undef ROUND_F
#undef ROUND_G
#undef ROUND_H
#undef ROUND_I
#undef RF4
#undef RG4
#undef RH4
#undef RI4
}
#endif


#ifdef __AVX__
static HEDLEY_ALWAYS_INLINE void md5_process_block_x2_avx(__m128i* state, const char* const* HEDLEY_RESTRICT data, size_t offset) {
	FN_VARS;
	
	// can use vmovddup instead?
#define LOADK(offs) \
	"vmovdqa %[k" offs "], %[TMPI2]\n" \
	"vpunpcklqdq %[TMPI2], %[TMPI2], %[TMPI1]\n" \
	"vpunpckhqdq %[TMPI2], %[TMPI2], %[TMPI2]\n"
#define ROUND_X(A, B, I, R) \
	"vpaddd " I ", %[" STR(A) "], %[" STR(A) "]\n" \
	"vpaddd %[TMPF1], %[" STR(A) "], %[" STR(A) "]\n" \
	"vpshufd $0b10100000, %[" STR(A) "], %[" STR(A) "]\n" \
	"vpsrlq $" STR(R) ", %[" STR(A) "], %[" STR(A) "]\n" \
	"vpaddd %[" STR(B) "], %[" STR(A) "], %[" STR(A) "]\n"

#ifdef PLATFORM_AMD64
#define READ4 \
	"vmovdqu %[i0], %[cache0]\n" \
	"vmovdqu %[i1], %[TMPI1]\n" \
	"vpunpckhqdq %[TMPI1], %[cache0], %[cache1]\n" \
	"vpunpcklqdq %[TMPI1], %[cache0], %[cache0]\n" \
	LOADK("") \
	"vpaddd %[cache0], %[TMPI1], %[TMPI1]\n" \
	"vpaddd %[cache1], %[TMPI2], %[TMPI2]\n"
#else
#define READ4 \
	"vmovdqu %[i0], %[TMPF1]\n" \
	"vmovdqu %[i1], %[TMPI2]\n" \
	"vpunpckhqdq %[TMPI2], %[TMPF1], %[TMPF2]\n" \
	"vpunpcklqdq %[TMPI2], %[TMPF1], %[TMPF1]\n" \
	"vmovdqa %[TMPF1], %[scratch0]\n" \
	"vmovdqa %[TMPF2], %[scratch1]\n" \
	LOADK("") \
	"vpaddd %[TMPF1], %[TMPI1], %[TMPI1]\n" \
	"vpaddd %[TMPF2], %[TMPI2], %[TMPI2]\n"
#endif

#define ROUND_F(A, B, C, D, I, R) \
	"vpxor %[" STR(D) "], %[" STR(C) "], %[TMPF1]\n" \
	"vpand %[" STR(B) "], %[TMPF1], %[TMPF1]\n" \
	"vpxor %[" STR(D) "], %[TMPF1], %[TMPF1]\n" \
	ROUND_X(A, B, I, R)
#define ROUND_H(A, B, C, D, I, R) \
	"vpxor %[" STR(D) "], %[" STR(C) "], %[TMPF1]\n" \
	"vpxor %[" STR(B) "], %[TMPF1], %[TMPF1]\n" \
	ROUND_X(A, B, I, R)
#define ROUND_I(A, B, C, D, I, R) \
	"vpxor %[" STR(D) "], %[TMPF2], %[TMPF1]\n" \
	"vpor %[" STR(B) "], %[TMPF1], %[TMPF1]\n" \
	"vpxor %[" STR(C) "], %[TMPF1], %[TMPF1]\n" \
	ROUND_X(A, B, I, R)

#define ROUND_G(A, B, C, D, I, R) \
	"vpaddd " I ", %[" STR(A) "], %[" STR(A) "]\n" \
	"vpandn %[" STR(C) "], %[" STR(D) "], %[TMPF1]\n" \
	"vpaddd %[TMPF1], %[" STR(A) "], %[" STR(A) "]\n" \
	"vpand %[" STR(B) "], %[" STR(D) "], %[TMPF1]\n" \
	"vpaddd %[TMPF1], %[" STR(A) "], %[" STR(A) "]\n" \
	"vpshufd $0b10100000, %[" STR(A) "], %[" STR(A) "]\n" \
	"vpsrlq $" STR(R) ", %[" STR(A) "], %[" STR(A) "]\n" \
	"vpaddd %[" STR(B) "], %[" STR(A) "], %[" STR(A) "]\n"

#define RF4(offs, r1, r2) asm( \
	READ4 \
	ROUND_F(A, B, C, D, "%[TMPI1]", 25) \
	"vpsrlq $32, %[TMPI1], %[TMPI1]\n" \
	ROUND_F(D, A, B, C, "%[TMPI1]", 20) \
	ROUND_F(C, D, A, B, "%[TMPI2]", 15) \
	"vpsrlq $32, %[TMPI2], %[TMPI2]\n" \
	ROUND_F(B, C, D, A, "%[TMPI2]", 10) \
: ASM_PARAMS_F(offs, r1, r2));
	
	// BLENDPS is faster than PBLENDW on Haswell and later, same elsewhere
#ifdef PLATFORM_AMD64
#define BLENDD(r1, r2, target) \
	"vblendps $0b1010, %[input" STR(r2) "], %[input" STR(r1) "], " target "\n"
#else
#define BLENDD(r1, r2, target) \
	"vmovdqa %[input" STR(r1) "], " target "\n" \
	"vblendps $0b1010, %[input" STR(r2) "], " target ", " target "\n"
#endif
#define RG4(offs, rs, r1, r2) \
	BLENDD(r1, r2, "%[TMPF2]") \
	LOADK(STR(offs)) \
	"vpaddd %[input" STR(rs) "], %[TMPI1], %[TMPI1]\n" \
	"vpaddd %[TMPF2], %[TMPI2], %[TMPI2]\n" \
	"vpsrlq $32, %[TMPI1], %[TMPF2]\n" \
	\
	ROUND_G(A, B, C, D, "%[TMPF2]", 27) \
	ROUND_G(D, A, B, C, "%[TMPI2]", 23) \
	"vpsrlq $32, %[TMPI2], %[TMPI2]\n" \
	ROUND_G(C, D, A, B, "%[TMPI2]", 18) \
	ROUND_G(B, C, D, A, "%[TMPI1]", 12)
	
#define RH4(offs, r1, r2, r3, r4) \
	BLENDD(r2, r1, "%[TMPF1]") \
	BLENDD(r4, r3, "%[TMPF2]") \
	LOADK(STR(offs)) \
	"vpaddd %[TMPF1], %[TMPI1], %[TMPI1]\n" \
	"vpaddd %[TMPF2], %[TMPI2], %[TMPI2]\n" \
	\
	"vpsrlq $32, %[TMPI1], %[TMPF2]\n" \
	ROUND_H(A, B, C, D, "%[TMPF2]", 28) \
	ROUND_H(D, A, B, C, "%[TMPI1]", 21) \
	"vpsrlq $32, %[TMPI2], %[TMPF2]\n" \
	ROUND_H(C, D, A, B, "%[TMPF2]", 16) \
	ROUND_H(B, C, D, A, "%[TMPI2]",  9)
	
#define RI4(offs, r1, r2, r3, r4) \
	BLENDD(r1, r2, "%[TMPF1]") \
	LOADK(STR(offs)) \
	"vpaddd %[TMPF1], %[TMPI1], %[TMPI1]\n" \
	BLENDD(r3, r4, "%[TMPF1]") \
	"vpaddd %[TMPF1], %[TMPI2], %[TMPI2]\n" \
	\
	ROUND_I(A, B, C, D, "%[TMPI1]", 26) \
	"vpsrlq $32, %[TMPI1], %[TMPI1]\n" \
	ROUND_I(D, A, B, C, "%[TMPI1]", 22) \
	ROUND_I(C, D, A, B, "%[TMPI2]", 17) \
	"vpsrlq $32, %[TMPI2], %[TMPI2]\n" \
	ROUND_I(B, C, D, A, "%[TMPI2]", 11)
	
	RF4(0, 0, 1)
	RF4(4, 2, 3)
	RF4(8, 4, 5)
	RF4(12, 6, 7)
	
	asm(
		RG4(0, 0, 3, 5)
		RG4(4, 2, 5, 7)
		RG4(8, 4, 7, 1)
		RG4(12, 6, 1, 3)
	: ASM_PARAMS(16));
	
	asm(
		RH4(0, 2, 4, 5, 7)
		RH4(4, 0, 2, 3, 5)
		RH4(8, 6, 0, 1, 3)
		RH4(12, 4, 6, 7, 1)
	: ASM_PARAMS(32));
	
	asm(
		"vpcmpeqb %[TMPF2], %[TMPF2], %[TMPF2]\n"
		RI4(0, 0, 3, 7, 2)
		RI4(4, 6, 1, 5, 0)
		RI4(8, 4, 7, 3, 6)
		RI4(12, 2, 5, 1, 4)
	: ASM_PARAMS(48));
	
	asm("" :: "x"(tmpI1), "x"(tmpI2), "x"(tmpF1), "x"(tmpF2) :); // for some reason, the above can fail without this
	
	state[0] = _mm_add_epi32(A, state[0]);
	state[1] = _mm_add_epi32(B, state[1]);
	state[2] = _mm_add_epi32(C, state[2]);
	state[3] = _mm_add_epi32(D, state[3]);
#undef ROUND_X
#undef ROUND_F
#undef ROUND_G
#undef ROUND_H
#undef ROUND_I
#undef BLENDD
}
#endif


#ifdef __AVX512VL__
static HEDLEY_ALWAYS_INLINE void md5_process_block_x2_avx512(__m128i* state, const char* const* HEDLEY_RESTRICT data, size_t offset) {
	FN_VARS;
	
#define ROUND_X(A, B, I, R) \
	"vpaddd " I ", %[" STR(A) "], %[" STR(A) "]\n" \
	"vpaddd %[TMPF1], %[" STR(A) "], %[" STR(A) "]\n" \
	"vprord $" STR(R) ", %[" STR(A) "], %[" STR(A) "]\n" \
	"vpaddd %[" STR(B) "], %[" STR(A) "], %[" STR(A) "]\n"

#define ROUND_F(A, B, C, D, I, R) \
	"vmovdqa %[" STR(D) "], %[TMPF1]\n" \
	"vpternlogd $0xD8, %[" STR(B) "], %[" STR(C) "], %[TMPF1]\n" \
	ROUND_X(A, B, I, R)
#define ROUND_G(A, B, C, D, I, R) \
	"vmovdqa %[" STR(D) "], %[TMPF1]\n" \
	"vpternlogd $0xAC, %[" STR(B) "], %[" STR(C) "], %[TMPF1]\n" \
	ROUND_X(A, B, I, R)
#define ROUND_H(A, B, C, D, I, R) \
	"vmovdqa %[" STR(D) "], %[TMPF1]\n" \
	"vpternlogd $0x96, %[" STR(B) "], %[" STR(C) "], %[TMPF1]\n" \
	ROUND_X(A, B, I, R)
#define ROUND_I(A, B, C, D, I, R) \
	"vmovdqa %[" STR(D) "], %[TMPF1]\n" \
	"vpternlogd $0x63, %[" STR(B) "], %[" STR(C) "], %[TMPF1]\n" \
	ROUND_X(A, B, I, R)

#ifdef PLATFORM_AMD64
#define BLENDD(r1, r2, target) \
	"vpblendd $0b1010, %[input" STR(r2) "], %[input" STR(r1) "], " target "\n"
#else
#define BLENDD(r1, r2, target) \
	"vmovdqa %[input" STR(r1) "], " target "\n" \
	"vpblendd $0b1010, %[input" STR(r2) "], " target ", " target "\n"
#endif
	
	RF4(0, 0, 1)
	RF4(4, 2, 3)
	RF4(8, 4, 5)
	RF4(12, 6, 7)
	
	asm(
		RG4(0, 0, 3, 5)
		RG4(4, 2, 5, 7)
		RG4(8, 4, 7, 1)
		RG4(12, 6, 1, 3)
	: ASM_PARAMS(16));
	
	asm(
		RH4(0, 2, 4, 5, 7)
		RH4(4, 0, 2, 3, 5)
		RH4(8, 6, 0, 1, 3)
		RH4(12, 4, 6, 7, 1)
	: ASM_PARAMS(32));
	
	asm(
		RI4(0, 0, 3, 7, 2)
		RI4(4, 6, 1, 5, 0)
		RI4(8, 4, 7, 3, 6)
		RI4(12, 2, 5, 1, 4)
	: ASM_PARAMS(48));
	
	asm("" :: "x"(tmpI1), "x"(tmpI2), "x"(tmpF1), "x"(tmpF2) :); // for some reason, the above can fail without this
	
	state[0] = _mm_add_epi32(A, state[0]);
	state[1] = _mm_add_epi32(B, state[1]);
	state[2] = _mm_add_epi32(C, state[2]);
	state[3] = _mm_add_epi32(D, state[3]);
#undef ROUND_X
#undef ROUND_F
#undef ROUND_G
#undef ROUND_H
#undef ROUND_I
#undef BLENDD
}
#endif

#ifdef __AVX__
#undef RF4
#undef RG4
#undef RH4
#undef RI4
#undef LOADK
#undef READ4
#endif

#undef ASM_PARAMS_F
#undef ASM_PARAMS
#undef FN_VARS
