/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) Enlightenment Development Team
 */

#include "equation_engine.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
equation_check(bool condition, const char *description)
{
   if (!condition)
     {
        fprintf(stderr, "Failed: %s\n", description);
        exit(1);
     }
}

static void
equation_expect(const char *expression, Equation_Mode mode, const char *expected)
{
   Equation_Result result = equation_evaluate(expression, mode, false);
   if (!result.valid || strcmp(result.decimal, expected))
     fprintf(stderr, "Expression: %s; mode: %d; expected: %s; got: %s; error: %s\n",
              expression, (int)mode, expected, result.decimal, result.error);
   equation_check(result.valid && !strcmp(result.decimal, expected), expression);
}

static void
equation_check_bidmas(void)
{
   const struct
   {
      const char *expression, *expected;
   } arithmetic[] = {
      /* Brackets override precedence, including nested groups. */
      { "2+3*4", "14" },
      { "(2+3)*4", "20" },
      { "2*(3+(4*5))", "46" },
      { "48/(2*(3+1))", "6" },
      /* Multiplication and division precede addition and subtraction. */
      { "20-12/3", "16" },
      { "24/3+2*5-1", "17" },
      { "2+3*4-8/2", "10" },
      /* Equal-precedence operators evaluate left to right. */
      { "24/3*2", "16" },
      { "24*3/2", "36" },
      { "64/4/2", "8" },
      { "10-3+2", "9" },
      { "10+3-2", "11" },
      { "20-5-3", "12" },
      /* Remainder shares multiplication/division precedence. */
      { "20%6*2", "4" },
      { "20*3%7", "4" },
      { "2+20%6", "4" },
   };
   for (size_t i = 0; i < sizeof(arithmetic) / sizeof(*arithmetic); ++i)
     {
        equation_expect(arithmetic[i].expression, EQUATION_MODE_SCIENTIFIC,
                         arithmetic[i].expected);
        equation_expect(arithmetic[i].expression, EQUATION_MODE_PROGRAMMER,
                         arithmetic[i].expected);
     }

   const struct
   {
      const char *expression, *expected;
   } scientific[] = {
      /* Indices precede multiplication/division and associate to the right. */
      { "2+3^2*4", "38" },
      { "2*3^2", "18" },
      { "32/2^3", "4" },
      { "2^3^2", "512" },
      { "(2^3)^2", "64" },
      { "(2+3)^2", "25" },
      /* A leading sign applies after a power; signed exponents are valid. */
      { "-2^2", "-4" },
      { "(-2)^2", "4" },
      { "2^-3", "0.125" },
      { "2^-2^2", "0.0625" },
      { "-2^2+3*4", "8" },
      { "sqrt(9+16)*2^2", "20" },
      { "3+4*2/(1-5)^2^3", "3.0001220703125" },
   };
   for (size_t i = 0; i < sizeof(scientific) / sizeof(*scientific); ++i)
     equation_expect(scientific[i].expression, EQUATION_MODE_SCIENTIFIC,
                      scientific[i].expected);

   /* Programmer mode uses ^ for XOR, below arithmetic precedence. */
   equation_expect("2+3^2*4", EQUATION_MODE_PROGRAMMER, "13");
   equation_expect("(2+3^2)*4", EQUATION_MODE_PROGRAMMER, "28");
}

int
main(void)
{
   equation_check_bidmas();
   const struct
   {
      const char *expression;
      unsigned base;
      const char *expected;
   } base_cases[] = { { "FF + 1", 16, "256" },
                       { "deadBEEF", 16, "3735928559" },
                       { "1010 + 1", 2, "11" },
                       { "42 + 1", 10, "43" },
                       { "0xFF + 0b10 + 0d10", 2, "267" },
                       { "FFFFFFFFFFFFFFFF", 16, "18446744073709551615" },
                       { "1111111111111111111111111111111111111111111111111111111111111111", 2,
                         "18446744073709551615" },
                       { "10 << 0d3", 2, "16" },
                       { "0x0B", 16, "11" },
                       { "00010", 10, "10" } };
   for (unsigned i = 0; i < sizeof(base_cases) / sizeof(*base_cases); ++i)
     {
        Equation_Result value = equation_evaluate_base(
          base_cases[i].expression, EQUATION_MODE_PROGRAMMER, false, base_cases[i].base);
        equation_check(value.valid && !strcmp(value.decimal, base_cases[i].expected),
                       base_cases[i].expression);
     }
   const struct
   {
      const char *expression;
      unsigned base;
   } invalid_bases[] = { { "102", 2 }, { "FF", 10 }, { "G", 16 },
                          { "0b", 16 }, { "0d", 2 },  { "10000000000000000", 16 },
                          { "1.1", 2 }, { "1", 8 } };
   for (unsigned i = 0; i < sizeof(invalid_bases) / sizeof(*invalid_bases); ++i)
     equation_check(!equation_evaluate_base(invalid_bases[i].expression, EQUATION_MODE_PROGRAMMER,
                                            false, invalid_bases[i].base)
                       .valid,
                    invalid_bases[i].expression);
   equation_expect("sqrt(81)+log(100)", EQUATION_MODE_SCIENTIFIC, "11");
   equation_expect("floor(2.9)", EQUATION_MODE_SCIENTIFIC, "2");
   equation_expect("floor(-2.1)", EQUATION_MODE_SCIENTIFIC, "-3");
   equation_expect("ceil(2.1)", EQUATION_MODE_SCIENTIFIC, "3");
   equation_expect("ceil(-2.9)", EQUATION_MODE_SCIENTIFIC, "-2");
   equation_expect("1e3/4", EQUATION_MODE_SCIENTIFIC, "250");
   equation_expect("0xFFFFFFFFFFFFFFFF", EQUATION_MODE_PROGRAMMER, "18446744073709551615");
   equation_expect("0xff & 0b1010", EQUATION_MODE_PROGRAMMER, "10");
   equation_expect("1 << 63", EQUATION_MODE_PROGRAMMER, "9223372036854775808");
   equation_expect("~0+1", EQUATION_MODE_PROGRAMMER, "0");
   equation_expect("7/2", EQUATION_MODE_PROGRAMMER, "3");
   equation_expect("5^3", EQUATION_MODE_PROGRAMMER, "6");
   equation_expect("0-1", EQUATION_MODE_PROGRAMMER, "18446744073709551615");
   Equation_Result result = equation_evaluate("sin(30)", EQUATION_MODE_SCIENTIFIC, true);
   equation_check(result.valid && fabsl(strtold(result.decimal, NULL) - 0.5L) < 1e-15L,
                  "sin degrees");
   const char *invalid_scientific[]
     = { "1/0", "sqrt(-1)", "ln(0)", "2+", "(3", "abc", "1 2", "2&3", ".", "sqrt(-1)^0" };
   for (size_t i = 0; i < sizeof(invalid_scientific) / sizeof(*invalid_scientific); i++)
     equation_check(!equation_evaluate(invalid_scientific[i], EQUATION_MODE_SCIENTIFIC, false).valid,
                    invalid_scientific[i]);
   const char *invalid_programmer[]
     = { "1<<64", "1>>65", "18446744073709551616", "0x", "0b2", "1.5", "2/0" };
   for (size_t i = 0; i < sizeof(invalid_programmer) / sizeof(*invalid_programmer); i++)
     equation_check(!equation_evaluate(invalid_programmer[i], EQUATION_MODE_PROGRAMMER, false).valid,
                    invalid_programmer[i]);
   result = equation_evaluate("-255", EQUATION_MODE_SCIENTIFIC, false);
   equation_check(!strcmp(result.hexadecimal, "-0xFF") && !strcmp(result.binary, "-11111111"),
                  "negative base conversions");
   result = equation_evaluate("0.5", EQUATION_MODE_SCIENTIFIC, false);
   equation_check(result.valid && !strcmp(result.hexadecimal, "Requires an integer within 64 bits"),
                  "fraction conversion");
   result = equation_evaluate("0", EQUATION_MODE_PROGRAMMER, false);
   equation_check(!strcmp(result.binary, "0"), "zero binary without prefix");
   result = equation_evaluate("0b1010", EQUATION_MODE_PROGRAMMER, false);
   equation_check(!strcmp(result.binary, "1010"), "binary input retains prefix support");
   char nested[201];
   memset(nested, '(', 200);
   nested[200] = 0;
   equation_check(!equation_evaluate(nested, EQUATION_MODE_SCIENTIFIC, false).valid,
                  "recursion limit");
   equation_check(!equation_evaluate(NULL, EQUATION_MODE_PROGRAMMER, false).valid, "null input");
   return 0;
}
