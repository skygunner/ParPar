#ifndef GFMAT_COEFF_H
#define GFMAT_COEFF_H

#include "../src/hedley.h"
#include "../src/stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

void gfmat_init();
HEDLEY_CONST uint16_t gfmat_coeff(uint_fast16_t inputBlock, uint_fast16_t recoveryBlock);
HEDLEY_CONST uint16_t gf16_exp(uint_fast16_t v);

#ifdef __cplusplus
}
#endif

#endif
