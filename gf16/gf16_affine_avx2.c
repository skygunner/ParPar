
#include "gf16_global.h"
#include "platform.h"

#define MWORD_SIZE 32
#define _mword __m256i
#define _MM(f) _mm256_ ## f
#define _MMI(f) _mm256_ ## f ## _si256
#define _FN(f) f ## _avx2
#define _MM_END _mm256_zeroupper();

#if defined(__GFNI__) && defined(__AVX2__)
int gf16_affine_available_avx2 = 1;
# define _AVAILABLE 1
# include "gf16_shuffle_x86_prepare.h"
# include "gf16_checksum_x86.h"
#else
int gf16_affine_available_avx2 = 0;
#endif

#include "gf16_affine2x_x86.h"
#ifdef _AVAILABLE
# undef _AVAILABLE
#endif
#undef _MM_END
#undef _FN
#undef _MMI
#undef _MM
#undef _mword
#undef MWORD_SIZE


void gf16_affine_prepare_packed_avx2(void *HEDLEY_RESTRICT dst, const void *HEDLEY_RESTRICT src, size_t srcLen, size_t sliceLen, unsigned inputPackSize, unsigned inputNum, size_t chunkLen) {
#if defined(__GFNI__) && defined(__AVX2__)
	gf16_prepare_packed(dst, src, srcLen, sliceLen, sizeof(__m256i)*2, &gf16_shuffle_prepare_block_avx2, &gf16_shuffle_prepare_blocku_avx2, inputPackSize, inputNum, chunkLen,
#ifdef PLATFORM_AMD64
		3
#else
		1
#endif
	, NULL, NULL, NULL, NULL, NULL);
	_mm256_zeroupper();
#else
	UNUSED(dst); UNUSED(src); UNUSED(srcLen); UNUSED(sliceLen); UNUSED(inputPackSize); UNUSED(inputNum); UNUSED(chunkLen);
#endif
}

void gf16_affine_prepare_packed_cksum_avx2(void *HEDLEY_RESTRICT dst, const void *HEDLEY_RESTRICT src, size_t srcLen, size_t sliceLen, unsigned inputPackSize, unsigned inputNum, size_t chunkLen) {
#if defined(__GFNI__) && defined(__AVX2__)
	__m256i checksum = _mm256_setzero_si256();
	gf16_prepare_packed(dst, src, srcLen, sliceLen, sizeof(__m256i)*2, &gf16_shuffle_prepare_block_avx2, &gf16_shuffle_prepare_blocku_avx2, inputPackSize, inputNum, chunkLen,
#ifdef PLATFORM_AMD64
		3
#else
		1
#endif
	, &checksum, &gf16_checksum_block_avx2, &gf16_checksum_blocku_avx2, &gf16_checksum_zeroes_avx2, &gf16_checksum_prepare_avx2);
	_mm256_zeroupper();
#else
	UNUSED(dst); UNUSED(src); UNUSED(srcLen); UNUSED(sliceLen); UNUSED(inputPackSize); UNUSED(inputNum); UNUSED(chunkLen);
#endif
}


#if defined(__GFNI__) && defined(__AVX2__)
static HEDLEY_ALWAYS_INLINE __m256i gf16_affine_load_matrix(const void *HEDLEY_RESTRICT scratch, uint16_t coefficient) {
	__m256i depmask = _mm256_xor_si256(
		_mm256_load_si256((__m256i*)scratch + (coefficient & 0xf)*4),
		_mm256_load_si256((__m256i*)((char*)scratch + ((coefficient << 3) & 0x780)) + 1)
	);
	depmask = _mm256_xor_si256(depmask, _mm256_load_si256((__m256i*)((char*)scratch + ((coefficient >> 1) & 0x780)) + 2));
	depmask = _mm256_xor_si256(depmask, _mm256_load_si256((__m256i*)((char*)scratch + ((coefficient >> 5) & 0x780)) + 3));
	return depmask;
}
#endif

void gf16_affine_mul_avx2(const void *HEDLEY_RESTRICT scratch, void *HEDLEY_RESTRICT dst, const void *HEDLEY_RESTRICT src, size_t len, uint16_t coefficient, void *HEDLEY_RESTRICT mutScratch) {
	UNUSED(mutScratch);
#if defined(__GFNI__) && defined(__AVX2__)
	__m256i depmask = gf16_affine_load_matrix(scratch, coefficient);
	
	__m256i mat_ll = _mm256_broadcastq_epi64(_mm256_castsi256_si128(depmask));
	__m256i mat_hh = _mm256_permute4x64_epi64(depmask, _MM_SHUFFLE(1,1,1,1));
	__m256i mat_lh = _mm256_permute4x64_epi64(depmask, _MM_SHUFFLE(3,3,3,3));
	__m256i mat_hl = _mm256_permute4x64_epi64(depmask, _MM_SHUFFLE(2,2,2,2));
	
	
	uint8_t* _src = (uint8_t*)src + len;
	uint8_t* _dst = (uint8_t*)dst + len;
	
	for(intptr_t ptr = -(intptr_t)len; ptr; ptr += sizeof(__m256i)*2) {
		__m256i ta = _mm256_load_si256((__m256i*)(_src + ptr));
		__m256i tb = _mm256_load_si256((__m256i*)(_src + ptr) + 1);

		__m256i tpl = _mm256_xor_si256(
			_mm256_gf2p8affine_epi64_epi8(ta, mat_lh, 0),
			_mm256_gf2p8affine_epi64_epi8(tb, mat_ll, 0)
		);
		__m256i tph = _mm256_xor_si256(
			_mm256_gf2p8affine_epi64_epi8(ta, mat_hh, 0),
			_mm256_gf2p8affine_epi64_epi8(tb, mat_hl, 0)
		);

		_mm256_store_si256 ((__m256i*)(_dst + ptr), tph);
		_mm256_store_si256 ((__m256i*)(_dst + ptr) + 1, tpl);
	}
	_mm256_zeroupper();
#else
	UNUSED(scratch); UNUSED(dst); UNUSED(src); UNUSED(len); UNUSED(coefficient);
#endif
}

#if defined(__GFNI__) && defined(__AVX2__)
static HEDLEY_ALWAYS_INLINE void gf16_affine_muladd_round(const __m256i* src, __m256i* tpl, __m256i* tph, __m256i mat_ll, __m256i mat_hl, __m256i mat_lh, __m256i mat_hh) {
	__m256i ta = _mm256_load_si256(src);
	__m256i tb = _mm256_load_si256(src + 1);
	*tpl = _mm256_xor_si256(*tpl, _mm256_gf2p8affine_epi64_epi8(ta, mat_lh, 0));
	*tpl = _mm256_xor_si256(*tpl, _mm256_gf2p8affine_epi64_epi8(tb, mat_ll, 0));
	*tph = _mm256_xor_si256(*tph, _mm256_gf2p8affine_epi64_epi8(ta, mat_hh, 0));
	*tph = _mm256_xor_si256(*tph, _mm256_gf2p8affine_epi64_epi8(tb, mat_hl, 0));
}
#include "gf16_muladd_multi.h"
static HEDLEY_ALWAYS_INLINE void gf16_affine_muladd_x_avx2(
	const void *HEDLEY_RESTRICT scratch,
	uint8_t *HEDLEY_RESTRICT _dst, const unsigned srcScale,
	GF16_MULADD_MULTI_SRCLIST, size_t len,
	const uint16_t *HEDLEY_RESTRICT coefficients, const int doPrefetch, const char* _pf
) {
	GF16_MULADD_MULTI_SRC_UNUSED(3);
	__m256i depmask = gf16_affine_load_matrix(scratch, coefficients[0]);
	
	__m256i mat_All = _mm256_broadcastq_epi64(_mm256_castsi256_si128(depmask));
	__m256i mat_Ahh = _mm256_permute4x64_epi64(depmask, _MM_SHUFFLE(1,1,1,1));
	__m256i mat_Alh = _mm256_permute4x64_epi64(depmask, _MM_SHUFFLE(3,3,3,3));
	__m256i mat_Ahl = _mm256_permute4x64_epi64(depmask, _MM_SHUFFLE(2,2,2,2));
	
	__m256i mat_Bll, mat_Bhh, mat_Bhl, mat_Blh;
	if(srcCount >= 2) {
		depmask = gf16_affine_load_matrix(scratch, coefficients[1]);
		mat_Bll = _mm256_broadcastq_epi64(_mm256_castsi256_si128(depmask));
		mat_Bhh = _mm256_permute4x64_epi64(depmask, _MM_SHUFFLE(1,1,1,1));
		mat_Blh = _mm256_permute4x64_epi64(depmask, _MM_SHUFFLE(3,3,3,3));
		mat_Bhl = _mm256_permute4x64_epi64(depmask, _MM_SHUFFLE(2,2,2,2));
	}
	
	__m256i mat_Cll, mat_Chh, mat_Chl, mat_Clh;
	if(srcCount > 2) {
		depmask = gf16_affine_load_matrix(scratch, coefficients[2]);
		mat_Cll = _mm256_broadcastq_epi64(_mm256_castsi256_si128(depmask));
		mat_Chh = _mm256_permute4x64_epi64(depmask, _MM_SHUFFLE(1,1,1,1));
		mat_Clh = _mm256_permute4x64_epi64(depmask, _MM_SHUFFLE(3,3,3,3));
		mat_Chl = _mm256_permute4x64_epi64(depmask, _MM_SHUFFLE(2,2,2,2));
	}
	
	for(intptr_t ptr = -(intptr_t)len; ptr; ptr += sizeof(__m256i)*2) {
		__m256i tph = _mm256_load_si256((__m256i*)(_dst + ptr));
		__m256i tpl = _mm256_load_si256((__m256i*)(_dst + ptr) + 1);
		gf16_affine_muladd_round((__m256i*)(_src1 + ptr*srcScale), &tpl, &tph, mat_All, mat_Ahl, mat_Alh, mat_Ahh);
		if(srcCount > 1)
			gf16_affine_muladd_round((__m256i*)(_src2 + ptr*srcScale), &tpl, &tph, mat_Bll, mat_Bhl, mat_Blh, mat_Bhh);
		if(srcCount > 2)
			gf16_affine_muladd_round((__m256i*)(_src3 + ptr*srcScale), &tpl, &tph, mat_Cll, mat_Chl, mat_Clh, mat_Chh);
		_mm256_store_si256 ((__m256i*)(_dst + ptr), tph);
		_mm256_store_si256 ((__m256i*)(_dst + ptr)+1, tpl);
		
		if(doPrefetch == 1)
			_mm_prefetch(_pf+ptr, MM_HINT_WT1);
		if(doPrefetch == 2)
			_mm_prefetch(_pf+ptr, _MM_HINT_T2);
	}
}
#endif /*defined(__GFNI__) && defined(__AVX2__)*/


void gf16_affine_muladd_avx2(const void *HEDLEY_RESTRICT scratch, void *HEDLEY_RESTRICT dst, const void *HEDLEY_RESTRICT src, size_t len, uint16_t coefficient, void *HEDLEY_RESTRICT mutScratch) {
	UNUSED(mutScratch);
#if defined(__GFNI__) && defined(__AVX2__)
	gf16_muladd_single(scratch, &gf16_affine_muladd_x_avx2, dst, src, len, coefficient);
	_mm256_zeroupper();
#else
	UNUSED(scratch); UNUSED(dst); UNUSED(src); UNUSED(len); UNUSED(coefficient);
#endif
}

void gf16_affine_muladd_prefetch_avx2(const void *HEDLEY_RESTRICT scratch, void *HEDLEY_RESTRICT dst, const void *HEDLEY_RESTRICT src, size_t len, uint16_t coefficient, void *HEDLEY_RESTRICT mutScratch, const void *HEDLEY_RESTRICT prefetch) {
	UNUSED(mutScratch);
#if defined(__GFNI__) && defined(__AVX2__)
	gf16_muladd_prefetch_single(scratch, &gf16_affine_muladd_x_avx2, dst, src, len, coefficient, prefetch);
	_mm256_zeroupper();
#else
	UNUSED(scratch); UNUSED(dst); UNUSED(src); UNUSED(len); UNUSED(coefficient); UNUSED(prefetch);
#endif
}

unsigned gf16_affine_muladd_multi_avx2(const void *HEDLEY_RESTRICT scratch, unsigned regions, size_t offset, void *HEDLEY_RESTRICT dst, const void* const*HEDLEY_RESTRICT src, size_t len, const uint16_t *HEDLEY_RESTRICT coefficients, void *HEDLEY_RESTRICT mutScratch) {
	UNUSED(mutScratch);
#if defined(__GFNI__) && defined(__AVX2__) && defined(PLATFORM_AMD64)
	unsigned region = gf16_muladd_multi(scratch, &gf16_affine_muladd_x_avx2, 3, regions, offset, dst, src, len, coefficients);
	_mm256_zeroupper();
	return region;
#else
	UNUSED(scratch); UNUSED(regions); UNUSED(offset); UNUSED(dst); UNUSED(src); UNUSED(len); UNUSED(coefficients);
	return 0;
#endif
}

unsigned gf16_affine_muladd_multi_packed_avx2(const void *HEDLEY_RESTRICT scratch, unsigned regions, void *HEDLEY_RESTRICT dst, const void* HEDLEY_RESTRICT src, size_t len, const uint16_t *HEDLEY_RESTRICT coefficients, void *HEDLEY_RESTRICT mutScratch) {
	UNUSED(mutScratch);
#if defined(__GFNI__) && defined(__AVX2__) && defined(PLATFORM_AMD64)
	unsigned region = gf16_muladd_multi_packed(scratch, &gf16_affine_muladd_x_avx2, 3, regions, dst, src, len, sizeof(__m256i)*2, coefficients);
	_mm256_zeroupper();
	return region;
#else
	UNUSED(scratch); UNUSED(regions); UNUSED(dst); UNUSED(src); UNUSED(len); UNUSED(coefficients);
	return 0;
#endif
}

void gf16_affine_muladd_multi_packpf_avx2(const void *HEDLEY_RESTRICT scratch, unsigned regions, void *HEDLEY_RESTRICT dst, const void* HEDLEY_RESTRICT src, size_t len, const uint16_t *HEDLEY_RESTRICT coefficients, void *HEDLEY_RESTRICT mutScratch, const void* HEDLEY_RESTRICT prefetchIn, const void* HEDLEY_RESTRICT prefetchOut) {
	UNUSED(mutScratch);
#if defined(__GFNI__) && defined(__AVX2__) && defined(PLATFORM_AMD64)
	gf16_muladd_multi_packpf(scratch, &gf16_affine_muladd_x_avx2, 3, regions, dst, src, len, sizeof(__m256i)*2, coefficients, 0, prefetchIn, prefetchOut);
	_mm256_zeroupper();
#else
	UNUSED(scratch); UNUSED(regions); UNUSED(dst); UNUSED(src); UNUSED(len); UNUSED(coefficients); UNUSED(prefetchIn); UNUSED(prefetchOut);
#endif
}


#if defined(__GFNI__) && defined(__AVX2__)
# include "gf16_bitdep_init_avx2.h"
#endif
void* gf16_affine_init_avx2(int polynomial) {
#if defined(__GFNI__) && defined(__AVX2__)
	__m256i* ret;
	ALIGN_ALLOC(ret, sizeof(__m256i)*16*4, 32);
	gf16_bitdep_init256(ret, polynomial, 1);
	return ret;
#else
	UNUSED(polynomial);
	return NULL;
#endif
}



#if defined(__GFNI__) && defined(__AVX2__)
static HEDLEY_ALWAYS_INLINE void gf16_affine2x_muladd_x_avx2(
	const void *HEDLEY_RESTRICT scratch,
	uint8_t *HEDLEY_RESTRICT _dst, const unsigned srcScale,
	GF16_MULADD_MULTI_SRCLIST,
	size_t len, const uint16_t *HEDLEY_RESTRICT coefficients, const int doPrefetch, const char* _pf
) {
	GF16_MULADD_MULTI_SRC_UNUSED(6);
	
	__m256i matNormA, matSwapA;
	__m256i matNormB, matSwapB;
	__m256i matNormC, matSwapC;
	__m256i matNormD, matSwapD;
	__m256i matNormE, matSwapE;
	__m256i matNormF, matSwapF;
	
	__m256i depmask = gf16_affine_load_matrix(scratch, coefficients[0]);
	matNormA = _mm256_inserti128_si256(depmask, _mm256_castsi256_si128(depmask), 1);
	matSwapA = _mm256_permute2x128_si256(depmask, depmask, 0x11);
	if(srcCount >= 2) {
		depmask = gf16_affine_load_matrix(scratch, coefficients[1]);
		matNormB = _mm256_inserti128_si256(depmask, _mm256_castsi256_si128(depmask), 1);
		matSwapB = _mm256_permute2x128_si256(depmask, depmask, 0x11);
	}
	if(srcCount >= 3) {
		depmask = gf16_affine_load_matrix(scratch, coefficients[2]);
		matNormC = _mm256_inserti128_si256(depmask, _mm256_castsi256_si128(depmask), 1);
		matSwapC = _mm256_permute2x128_si256(depmask, depmask, 0x11);
	}
	if(srcCount >= 4) {
		depmask = gf16_affine_load_matrix(scratch, coefficients[3]);
		matNormD = _mm256_inserti128_si256(depmask, _mm256_castsi256_si128(depmask), 1);
		matSwapD = _mm256_permute2x128_si256(depmask, depmask, 0x11);
	}
	if(srcCount >= 5) {
		depmask = gf16_affine_load_matrix(scratch, coefficients[4]);
		matNormE = _mm256_inserti128_si256(depmask, _mm256_castsi256_si128(depmask), 1);
		matSwapE = _mm256_permute2x128_si256(depmask, depmask, 0x11);
	}
	if(srcCount >= 6) {
		depmask = gf16_affine_load_matrix(scratch, coefficients[5]);
		matNormF = _mm256_inserti128_si256(depmask, _mm256_castsi256_si128(depmask), 1);
		matSwapF = _mm256_permute2x128_si256(depmask, depmask, 0x11);
	}
	
	
	intptr_t ptr = -(intptr_t)len;
	if(doPrefetch) {
		if(doPrefetch == 1)
			_mm_prefetch(_pf+ptr, MM_HINT_WT1);
		if(doPrefetch == 2)
			_mm_prefetch(_pf+ptr, _MM_HINT_T2);
		if(ptr & (sizeof(__m256i)*2-1)) { // align to a cacheline boundary
			__m256i data = _mm256_load_si256((__m256i*)(_src1 + ptr*srcScale));
			__m256i result1 = _mm256_gf2p8affine_epi64_epi8(data, matNormA, 0);
			__m256i result2 = _mm256_gf2p8affine_epi64_epi8(data, matSwapA, 0);
			
			if(srcCount >= 2) {
				data = _mm256_load_si256((__m256i*)(_src2 + ptr*srcScale));
				result1 = _mm256_xor_si256(result1, _mm256_gf2p8affine_epi64_epi8(data, matNormB, 0));
				result2 = _mm256_xor_si256(result2, _mm256_gf2p8affine_epi64_epi8(data, matSwapB, 0));
			}
			
			if(srcCount >= 3) {
				data = _mm256_load_si256((__m256i*)(_src3 + ptr*srcScale));
				result1 = _mm256_xor_si256(result1, _mm256_gf2p8affine_epi64_epi8(data, matNormC, 0));
				result2 = _mm256_xor_si256(result2, _mm256_gf2p8affine_epi64_epi8(data, matSwapC, 0));
			}
			if(srcCount >= 4) {
				data = _mm256_load_si256((__m256i*)(_src4 + ptr*srcScale));
				result1 = _mm256_xor_si256(result1, _mm256_gf2p8affine_epi64_epi8(data, matNormD, 0));
				result2 = _mm256_xor_si256(result2, _mm256_gf2p8affine_epi64_epi8(data, matSwapD, 0));
			}
			if(srcCount >= 5) {
				data = _mm256_load_si256((__m256i*)(_src5 + ptr*srcScale));
				result1 = _mm256_xor_si256(result1, _mm256_gf2p8affine_epi64_epi8(data, matNormE, 0));
				result2 = _mm256_xor_si256(result2, _mm256_gf2p8affine_epi64_epi8(data, matSwapE, 0));
			}
			if(srcCount >= 6) {
				data = _mm256_load_si256((__m256i*)(_src6 + ptr*srcScale));
				result1 = _mm256_xor_si256(result1, _mm256_gf2p8affine_epi64_epi8(data, matNormF, 0));
				result2 = _mm256_xor_si256(result2, _mm256_gf2p8affine_epi64_epi8(data, matSwapF, 0));
			}
			
			result1 = _mm256_xor_si256(result1, _mm256_load_si256((__m256i*)(_dst + ptr)));
			result1 = _mm256_xor_si256(result1, _mm256_shuffle_epi32(result2, _MM_SHUFFLE(1,0,3,2)));
			_mm256_store_si256((__m256i*)(_dst + ptr), result1);
			
			ptr += sizeof(__m256i);
		}
	}
	while(ptr) {
		if(doPrefetch == 1)
			_mm_prefetch(_pf+ptr, MM_HINT_WT1);
		if(doPrefetch == 2)
			_mm_prefetch(_pf+ptr, _MM_HINT_T2);
		
		for(int iter=0; iter<(doPrefetch?2:1); iter++) { // if prefetching, iterate on cachelines
			__m256i data = _mm256_load_si256((__m256i*)(_src1 + ptr*srcScale));
			__m256i result1 = _mm256_gf2p8affine_epi64_epi8(data, matNormA, 0);
			__m256i result2 = _mm256_gf2p8affine_epi64_epi8(data, matSwapA, 0);
			
			if(srcCount >= 2) {
				data = _mm256_load_si256((__m256i*)(_src2 + ptr*srcScale));
				result1 = _mm256_xor_si256(result1, _mm256_gf2p8affine_epi64_epi8(data, matNormB, 0));
				result2 = _mm256_xor_si256(result2, _mm256_gf2p8affine_epi64_epi8(data, matSwapB, 0));
			}
			if(srcCount >= 3) {
				data = _mm256_load_si256((__m256i*)(_src3 + ptr*srcScale));
				result1 = _mm256_xor_si256(result1, _mm256_gf2p8affine_epi64_epi8(data, matNormC, 0));
				result2 = _mm256_xor_si256(result2, _mm256_gf2p8affine_epi64_epi8(data, matSwapC, 0));
			}
			if(srcCount >= 4) {
				data = _mm256_load_si256((__m256i*)(_src4 + ptr*srcScale));
				result1 = _mm256_xor_si256(result1, _mm256_gf2p8affine_epi64_epi8(data, matNormD, 0));
				result2 = _mm256_xor_si256(result2, _mm256_gf2p8affine_epi64_epi8(data, matSwapD, 0));
			}
			if(srcCount >= 5) {
				data = _mm256_load_si256((__m256i*)(_src5 + ptr*srcScale));
				result1 = _mm256_xor_si256(result1, _mm256_gf2p8affine_epi64_epi8(data, matNormE, 0));
				result2 = _mm256_xor_si256(result2, _mm256_gf2p8affine_epi64_epi8(data, matSwapE, 0));
			}
			if(srcCount >= 6) {
				data = _mm256_load_si256((__m256i*)(_src6 + ptr*srcScale));
				result1 = _mm256_xor_si256(result1, _mm256_gf2p8affine_epi64_epi8(data, matNormF, 0));
				result2 = _mm256_xor_si256(result2, _mm256_gf2p8affine_epi64_epi8(data, matSwapF, 0));
			}
			
			result1 = _mm256_xor_si256(result1, _mm256_load_si256((__m256i*)(_dst + ptr)));
			result1 = _mm256_xor_si256(result1, _mm256_shuffle_epi32(result2, _MM_SHUFFLE(1,0,3,2)));
			_mm256_store_si256((__m256i*)(_dst + ptr), result1);
			
			ptr += sizeof(__m256i);
		}
	}
}
#endif /*defined(__GFNI__) && defined(__AVX2__)*/

void gf16_affine2x_muladd_avx2(const void *HEDLEY_RESTRICT scratch, void *HEDLEY_RESTRICT dst, const void *HEDLEY_RESTRICT src, size_t len, uint16_t coefficient, void *HEDLEY_RESTRICT mutScratch) {
	UNUSED(mutScratch);
#if defined(__GFNI__) && defined(__AVX2__)
	gf16_muladd_single(scratch, &gf16_affine2x_muladd_x_avx2, dst, src, len, coefficient);
	_mm256_zeroupper();
#else
	UNUSED(scratch); UNUSED(dst); UNUSED(src); UNUSED(len); UNUSED(coefficient);
#endif
}

unsigned gf16_affine2x_muladd_multi_avx2(const void *HEDLEY_RESTRICT scratch, unsigned regions, size_t offset, void *HEDLEY_RESTRICT dst, const void* const*HEDLEY_RESTRICT src, size_t len, const uint16_t *HEDLEY_RESTRICT coefficients, void *HEDLEY_RESTRICT mutScratch) {
	UNUSED(mutScratch);
#if defined(__GFNI__) && defined(__AVX2__)
# ifdef PLATFORM_AMD64
	unsigned region = gf16_muladd_multi(scratch, &gf16_affine2x_muladd_x_avx2, 6, regions, offset, dst, src, len, coefficients);
# else
	// if only 8 registers available, only allow 2 parallel regions
	unsigned region = gf16_muladd_multi(scratch, &gf16_affine2x_muladd_x_avx2, 2, regions, offset, dst, src, len, coefficients);
# endif
	_mm256_zeroupper();
	return region;
#else
	UNUSED(scratch); UNUSED(regions); UNUSED(offset); UNUSED(dst); UNUSED(src); UNUSED(len); UNUSED(coefficients);
	return 0;
#endif
}

unsigned gf16_affine2x_muladd_multi_packed_avx2(const void *HEDLEY_RESTRICT scratch, unsigned regions, void *HEDLEY_RESTRICT dst, const void* HEDLEY_RESTRICT src, size_t len, const uint16_t *HEDLEY_RESTRICT coefficients, void *HEDLEY_RESTRICT mutScratch) {
	UNUSED(mutScratch);
#if defined(__GFNI__) && defined(__AVX2__)
# ifdef PLATFORM_AMD64
	unsigned region = gf16_muladd_multi_packed(scratch, &gf16_affine2x_muladd_x_avx2, 6, regions, dst, src, len, sizeof(__m256i), coefficients);
# else
	// if only 8 registers available, only allow 2 parallel regions
	unsigned region = gf16_muladd_multi_packed(scratch, &gf16_affine2x_muladd_x_avx2, 2, regions, dst, src, len, sizeof(__m256i), coefficients);
# endif
	_mm256_zeroupper();
	return region;
#else
	UNUSED(scratch); UNUSED(regions); UNUSED(dst); UNUSED(src); UNUSED(len); UNUSED(coefficients);
	return 0;
#endif
}

void gf16_affine2x_muladd_multi_packpf_avx2(const void *HEDLEY_RESTRICT scratch, unsigned regions, void *HEDLEY_RESTRICT dst, const void* HEDLEY_RESTRICT src, size_t len, const uint16_t *HEDLEY_RESTRICT coefficients, void *HEDLEY_RESTRICT mutScratch, const void* HEDLEY_RESTRICT prefetchIn, const void* HEDLEY_RESTRICT prefetchOut) {
	UNUSED(mutScratch);
#if defined(__GFNI__) && defined(__AVX2__)
# ifdef PLATFORM_AMD64
	gf16_muladd_multi_packpf(scratch, &gf16_affine2x_muladd_x_avx2, 6, regions, dst, src, len, sizeof(__m256i), coefficients, 0, prefetchIn, prefetchOut);
# else
	gf16_muladd_multi_packpf(scratch, &gf16_affine2x_muladd_x_avx2, 2, regions, dst, src, len, sizeof(__m256i), coefficients, 0, prefetchIn, prefetchOut);
# endif
	_mm256_zeroupper();
#else
	UNUSED(scratch); UNUSED(regions); UNUSED(dst); UNUSED(src); UNUSED(len); UNUSED(coefficients); UNUSED(prefetchIn); UNUSED(prefetchOut);
#endif
}

