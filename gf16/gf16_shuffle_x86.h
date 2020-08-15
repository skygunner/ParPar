
#include "gf16_shuffle_x86_common.h"
#include "gf16_global.h"

#ifdef _AVAILABLE
int _FN(gf16_shuffle_available) = 1;
#include "gf16_shuffle_x86_prepare.h"
#else
int _FN(gf16_shuffle_available) = 0;
#endif

void _FN(gf16_shuffle_prepare)(void *HEDLEY_RESTRICT dst, const void *HEDLEY_RESTRICT src, size_t srcLen) {
#ifdef _AVAILABLE
	gf16_prepare(dst, src, srcLen, sizeof(_mword)*2, &_FN(gf16_shuffle_prepare_block), &_FN(gf16_shuffle_prepare_blocku));
	_MM_END
#else
	UNUSED(dst); UNUSED(src); UNUSED(srcLen);
#endif
}

void _FN(gf16_shuffle_prepare_packed)(void *HEDLEY_RESTRICT dst, const void *HEDLEY_RESTRICT src, size_t srcLen, size_t sliceLen, unsigned inputPackSize, unsigned inputNum, size_t chunkLen) {
#ifdef _AVAILABLE
	gf16_prepare_packed(dst, src, srcLen, sliceLen, sizeof(_mword)*2, &_FN(gf16_shuffle_prepare_block), &_FN(gf16_shuffle_prepare_blocku), inputPackSize, inputNum, chunkLen,
#if MWORD_SIZE==64 && defined(PLATFORM_AMD64)
		3
#else
		1
#endif
	);
	_MM_END
#else
	UNUSED(dst); UNUSED(src); UNUSED(srcLen); UNUSED(sliceLen); UNUSED(inputPackSize); UNUSED(inputNum); UNUSED(chunkLen);
#endif
}

void _FN(gf16_shuffle_finish)(void *HEDLEY_RESTRICT dst, size_t len) {
#ifdef _AVAILABLE
	gf16_finish(dst, len, sizeof(_mword)*2, &_FN(gf16_shuffle_finish_block));
	_MM_END
#else
	UNUSED(dst); UNUSED(len);
#endif
}

#if MWORD_SIZE >= 32
# ifdef _AVAILABLE
static HEDLEY_ALWAYS_INLINE __m256i mul16_vec256(__m256i poly, __m256i src) {
	__m256i prodHi = _mm256_and_si256(_mm256_set1_epi8(0xf), _mm256_srli_epi16(src, 4));
	__m256i idx = _mm256_inserti128_si256(prodHi, _mm256_castsi256_si128(prodHi), 1);
#  if MWORD_SIZE == 64
	src = _mm256_ternarylogic_epi32(
		zext128_256(_mm256_extracti128_si256(prodHi, 1)),
		_mm256_set1_epi8(0xf),
		_mm256_slli_epi16(src, 4),
		0xF2
	);
#  else
	__m256i prodLo = _mm256_slli_epi16(_mm256_and_si256(_mm256_set1_epi8(0xf), src), 4);
	src = _mm256_or_si256(prodLo, zext128_256(_mm256_extracti128_si256(prodHi, 1)));
#  endif
	return _mm256_xor_si256(src, _mm256_shuffle_epi8(poly, idx));
	// another idea with AVX512 is to keep each 4-bit part of the 16-bit results in a 128-bit lane, and shuffle lanes to handle the shift
}
# endif
# if MWORD_SIZE == 64
#  define BCAST_HI(v) _mm512_shuffle_i32x4(_mm512_castsi256_si512(v), _mm512_castsi256_si512(v), _MM_SHUFFLE(1,1,1,1))
#  define BCAST_LO(v) _mm512_shuffle_i32x4(_mm512_castsi256_si512(v), _mm512_castsi256_si512(v), _MM_SHUFFLE(0,0,0,0))
# else
#  define BCAST_HI(v) _mm256_permute4x64_epi64(v, _MM_SHUFFLE(3,2,3,2))
#  define BCAST_LO(v) _mm256_inserti128_si256(v, _mm256_castsi256_si128(v), 1)
# endif
#endif

#ifdef _AVAILABLE
static HEDLEY_ALWAYS_INLINE void gf16_shuffle_setup_vec(const void *HEDLEY_RESTRICT scratch, uint16_t val, _mword* low0, _mword* high0, _mword* low1, _mword* high1, _mword* low2, _mword* high2, _mword* low3, _mword* high3) {
	__m128i pd0, pd1;
	shuf0_vector(val, &pd0, &pd1);
	
#if MWORD_SIZE >= 32
	__m256i poly = _mm256_load_si256((__m256i*)scratch);
	__m256i prod = _mm256_inserti128_si256(_mm256_castsi128_si256(pd0), pd1, 1);
	prod = _mm256_shuffle_epi8(prod, _mm256_set_epi32(
		0x0f0d0b09, 0x07050301, 0x0e0c0a08, 0x06040200,
		0x0f0d0b09, 0x07050301, 0x0e0c0a08, 0x06040200
	));
	prod = _mm256_permute4x64_epi64(prod, _MM_SHUFFLE(2,0,3,1));
	*low0  = BCAST_HI(prod);
	*high0 = BCAST_LO(prod);

	prod = mul16_vec256(poly, prod);
	*low1  = BCAST_HI(prod);
	*high1 = BCAST_LO(prod);
	
	prod = mul16_vec256(poly, prod);
	*low2  = BCAST_HI(prod);
	*high2 = BCAST_LO(prod);
	
	prod = mul16_vec256(poly, prod);
	*low3  = BCAST_HI(prod);
	*high3 = BCAST_LO(prod);
#else
	pd0 = _mm_shuffle_epi8(pd0, _mm_set_epi32(0x0f0d0b09, 0x07050301, 0x0e0c0a08, 0x06040200));
	pd1 = _mm_shuffle_epi8(pd1, _mm_set_epi32(0x0f0d0b09, 0x07050301, 0x0e0c0a08, 0x06040200));
	*low0  = _mm_unpacklo_epi64(pd0, pd1);
	*high0 = _mm_unpackhi_epi64(pd0, pd1);
	
	__m128i polyl = _mm_load_si128((__m128i*)scratch + 1);
	__m128i polyh = _mm_load_si128((__m128i*)scratch);
	mul16_vec128(polyl, polyh, *low0, *high0, low1, high1);
	mul16_vec128(polyl, polyh, *low1, *high1, low2, high2);
	mul16_vec128(polyl, polyh, *low2, *high2, low3, high3);
#endif
}
#endif

void _FN(gf16_shuffle_mul)(const void *HEDLEY_RESTRICT scratch, void *HEDLEY_RESTRICT dst, const void *HEDLEY_RESTRICT src, size_t len, uint16_t val, void *HEDLEY_RESTRICT mutScratch) {
	UNUSED(mutScratch);
#ifdef _AVAILABLE
	_mword low0, low1, low2, low3, high0, high1, high2, high3;
	gf16_shuffle_setup_vec(scratch, val, &low0, &high0, &low1, &high1, &low2, &high2, &low3, &high3);
	
	_mword mask = _MM(set1_epi8) (0x0f);
	uint8_t* _src = (uint8_t*)src + len;
	uint8_t* _dst = (uint8_t*)dst + len;

	for(long ptr = -(long)len; ptr; ptr += sizeof(_mword)*2) {
		_mword ta = _MMI(load)((_mword*)(_src+ptr));
		_mword tb = _MMI(load)((_mword*)(_src+ptr) + 1);

		_mword ti = _MMI(and) (mask, tb);
		_mword tph = _MM(shuffle_epi8) (high0, ti);
		_mword tpl = _MM(shuffle_epi8) (low0, ti);

		ti = _MM_SRLI4_EPI8(tb);
#if MWORD_SIZE == 64
		_mword ti2 = _MMI(and) (mask, ta);
		tpl = _mm512_ternarylogic_epi32(tpl, _MM(shuffle_epi8) (low1, ti), _MM(shuffle_epi8) (low2, ti2), 0x96);
		tph = _mm512_ternarylogic_epi32(tph, _MM(shuffle_epi8) (high1, ti), _MM(shuffle_epi8) (high2, ti2), 0x96);
#else
		tpl = _MMI(xor)(_MM(shuffle_epi8) (low1, ti), tpl);
		tph = _MMI(xor)(_MM(shuffle_epi8) (high1, ti), tph);

		ti = _MMI(and) (mask, ta);
		tpl = _MMI(xor)(_MM(shuffle_epi8) (low2, ti), tpl);
		tph = _MMI(xor)(_MM(shuffle_epi8) (high2, ti), tph);
#endif
		ti = _MM_SRLI4_EPI8(ta);
		tpl = _MMI(xor)(_MM(shuffle_epi8) (low3, ti), tpl);
		tph = _MMI(xor)(_MM(shuffle_epi8) (high3, ti), tph);

		_MMI(store) ((_mword*)(_dst+ptr), tph);
		_MMI(store) ((_mword*)(_dst+ptr) + 1, tpl);
	}
	_MM_END
#else
	UNUSED(scratch); UNUSED(dst); UNUSED(src); UNUSED(len); UNUSED(val);
#endif
}


void _FN(gf16_shuffle_muladd)(const void *HEDLEY_RESTRICT scratch, void *HEDLEY_RESTRICT dst, const void *HEDLEY_RESTRICT src, size_t len, uint16_t val, void *HEDLEY_RESTRICT mutScratch) {
	UNUSED(mutScratch);
#ifdef _AVAILABLE
	_mword low0, low1, low2, low3, high0, high1, high2, high3;
	gf16_shuffle_setup_vec(scratch, val, &low0, &high0, &low1, &high1, &low2, &high2, &low3, &high3);
	
	_mword mask = _MM(set1_epi8) (0x0f);
	uint8_t* _src = (uint8_t*)src + len;
	uint8_t* _dst = (uint8_t*)dst + len;

	for(long ptr = -(long)len; ptr; ptr += sizeof(_mword)*2) {
		_mword ta = _MMI(load)((_mword*)(_src+ptr));
		_mword tb = _MMI(load)((_mword*)(_src+ptr) + 1);

		_mword ti = _MMI(and) (mask, tb);
		_mword tph = _MM(shuffle_epi8) (high0, ti);
		_mword tpl = _MM(shuffle_epi8) (low0, ti);

		ti = _MM_SRLI4_EPI8(tb);
#if MWORD_SIZE == 64
		tpl = _mm512_ternarylogic_epi32(tpl, _MM(shuffle_epi8) (low1, ti), _MMI(load)((_mword*)(_dst+ptr) + 1), 0x96);
		tph = _mm512_ternarylogic_epi32(tph, _MM(shuffle_epi8) (high1, ti), _MMI(load)((_mword*)(_dst+ptr)), 0x96);

		ti = _MMI(and) (mask, ta);
		_mword ti2 = _MMI(and) (mask, _MM(srli_epi16)(ta, 4));
		
		tpl = _mm512_ternarylogic_epi32(tpl, _MM(shuffle_epi8) (low2, ti), _MM(shuffle_epi8) (low3, ti2), 0x96);
		tph = _mm512_ternarylogic_epi32(tph, _MM(shuffle_epi8) (high2, ti), _MM(shuffle_epi8) (high3, ti2), 0x96);
#else
		tpl = _MMI(xor)(_MM(shuffle_epi8) (low1, ti), tpl);
		tph = _MMI(xor)(_MM(shuffle_epi8) (high1, ti), tph);

		tph = _MMI(xor)(tph, _MMI(load)((_mword*)(_dst+ptr)));
		tpl = _MMI(xor)(tpl, _MMI(load)((_mword*)(_dst+ptr) + 1));

		ti = _MMI(and) (mask, ta);
		tpl = _MMI(xor)(_MM(shuffle_epi8) (low2, ti), tpl);
		tph = _MMI(xor)(_MM(shuffle_epi8) (high2, ti), tph);

		ti = _MM_SRLI4_EPI8(ta);
		tpl = _MMI(xor)(_MM(shuffle_epi8) (low3, ti), tpl);
		tph = _MMI(xor)(_MM(shuffle_epi8) (high3, ti), tph);
#endif
		
		_MMI(store) ((_mword*)(_dst+ptr), tph);
		_MMI(store) ((_mword*)(_dst+ptr) + 1, tpl);
	}
	_MM_END
#else
	UNUSED(scratch); UNUSED(dst); UNUSED(src); UNUSED(len); UNUSED(val);
#endif
}

