#include <string.h> // memcpy+memset
#include "../src/stdint.h"

#ifdef _MSC_VER
# ifndef __BYTE_ORDER__
#  define __BYTE_ORDER__ 1234
# endif
# ifndef __ORDER_BIG_ENDIAN__
#  define __ORDER_BIG_ENDIAN__ 4321
# endif
#endif

#define ADD(a, b) (a+b)
#define VAL(k) (k)
#define word_t uint32_t
#define INPUT(k, set, ptr, offs, idx, var) (var + k)
#if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
# define LOAD(k, set, ptr, offs, idx, var) (memcpy(&var, ((uint32_t*)(ptr[set])) + idx, 4), var + k)
#else
/* big-endian -> need to byteswap */
# define BSWAP(v) (((v&0xff) << 24) | ((v&0xff00) << 8) | ((v>>8) & 0xff00) | (v>>24))
# define LOAD(k, set, ptr, offs, idx, var) (memcpy(&var, ((uint32_t*)(ptr[set])) + idx, 4), var = BSWAP(var), var + k)
#endif


# if defined(_MSC_VER)
#  define ROTATE(a,n)   _lrotl(a,n)
# elif defined(__ICC)
#  define ROTATE(a,n)   _rotl(a,n)
# elif defined(__MWERKS__)
#  if defined(__POWERPC__)
#   define ROTATE(a,n)  __rlwinm(a,n,0,31)
#  elif defined(__MC68K__)
    /* Motorola specific tweak. <appro@fy.chalmers.se> */
#   define ROTATE(a,n)  ( n<24 ? __rol(a,n) : __ror(a,32-n) )
#  else
#   define ROTATE(a,n)  __rol(a,n)
#  endif
# elif defined(__GNUC__) && __GNUC__>=2 && !defined(OPENSSL_NO_ASM) && !defined(OPENSSL_NO_INLINE_ASM)
  /*
   * Some GNU C inline assembler templates. Note that these are
   * rotates by *constant* number of bits! But that's exactly
   * what we need here...
   *                                    <appro@fy.chalmers.se>
   */
#  if defined(__i386) || defined(__i386__) || defined(__x86_64) || defined(__x86_64__)
#   define ROTATE(a,n)  ({ unsigned int ret;   \
                                asm (                   \
                                "roll %1,%0"            \
                                : "=r"(ret)             \
                                : "I"(n), "0"((unsigned int)(a))        \
                                : "cc");                \
                           ret;                         \
                        })
#  elif defined(_ARCH_PPC) || defined(_ARCH_PPC64) || \
        defined(__powerpc) || defined(__ppc__) || defined(__powerpc64__)
#   define ROTATE(a,n)  ({ unsigned int ret;   \
                                asm (                   \
                                "rlwinm %0,%1,%2,0,31"  \
                                : "=r"(ret)             \
                                : "r"(a), "I"(n));      \
                           ret;                         \
                        })
#  elif defined(__s390x__)
#   define ROTATE(a,n) ({ unsigned int ret;    \
                                asm ("rll %0,%1,%2"     \
                                : "=r"(ret)             \
                                : "r"(a), "I"(n));      \
                          ret;                          \
                        })
#  endif
# endif
# ifndef ROTATE
#  define ROTATE(a,n)     (((a)<<(n))|(((a)&0xffffffff)>>(32-(n))))
# endif


#define F(b,c,d) (((c ^ d) & b) ^ d)
#define G(b,c,d) ((d & b) | (~d & c))
#define H(b,c,d) (d ^ c ^ b)
#define I(b,c,d) ((~d | b) ^ c)

