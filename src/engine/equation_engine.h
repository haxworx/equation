/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) Enlightenment Development Team
 */

#ifndef EQUATION_ENGINE_H
#define EQUATION_ENGINE_H
#include <stdbool.h>

typedef enum
{
   EQUATION_MODE_SCIENTIFIC,
   EQUATION_MODE_PROGRAMMER
} Equation_Mode;

typedef struct
{
   bool valid;
   char decimal[128];
   char hexadecimal[128];
   char binary[128];
   char error[192];
} Equation_Result;

/* Programmer arithmetic wraps modulo 2^64. ^ means XOR in programmer mode
 * and exponentiation in scientific mode. Scientific base conversions require
 * an exact integer whose magnitude fits in 64 bits. No UI dependencies. */
Equation_Result equation_evaluate(const char *expression, Equation_Mode mode, bool degrees);
/* Unprefixed programmer literals use base (2, 10 or 16). Explicit 0b, 0d
 * and 0x prefixes override it. Scientific syntax is independent of base. */
Equation_Result equation_evaluate_base(const char *expression, Equation_Mode mode, bool degrees,
                                       unsigned base);
#endif
