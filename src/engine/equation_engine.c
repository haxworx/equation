/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) Enlightenment Development Team
 */

#include "equation_engine.h"
#include "equation_intl.h"
#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
   long double real;
   uint64_t bits;
} Equation_Value;
typedef struct
{
   const char *input;
   size_t position;
   unsigned depth, base;
   bool integer, degrees;
   Equation_Result *result;
} Equation_Parser;

static Equation_Value equation_expression(Equation_Parser *parser, int minimum);

static void
equation_fail(Equation_Parser *parser, const char *message)
{
   if (!parser->result->error[0])
     snprintf(parser->result->error, sizeof(parser->result->error), _("%s at column %zu"), message,
              parser->position + 1);
}

static void
equation_space(Equation_Parser *parser)
{
   while (isspace((unsigned char)parser->input[parser->position]))
     parser->position++;
}

static bool
equation_take(Equation_Parser *parser, const char *token)
{
   equation_space(parser);
   size_t length = strlen(token);
   if (strncmp(parser->input + parser->position, token, length)) return false;
   parser->position += length;
   return true;
}

static Equation_Value
equation_primary(Equation_Parser *parser)
{
   Equation_Value value = { 0 };
   if (parser->result->error[0]) return value;
   if (++parser->depth > 128)
     {
        equation_fail(parser, _("Expression is too deeply nested"));
        parser->depth--;
        return value;
     }
   if (equation_take(parser, "+"))
     value = equation_primary(parser);
   else if (equation_take(parser, "-"))
     {
        value = equation_expression(parser, 7);
        if (parser->integer)
          value.bits = UINT64_C(0) - value.bits;
        else
          value.real = -value.real;
     }
   else if (equation_take(parser, "~"))
     {
        value = equation_primary(parser);
        if (parser->integer)
          value.bits = ~value.bits;
        else
          equation_fail(parser, _("Bitwise NOT requires programmer mode"));
     }
   else if (equation_take(parser, "("))
     {
        value = equation_expression(parser, 0);
        if (!equation_take(parser, ")")) equation_fail(parser, _("Expected closing parenthesis"));
     }
   else
     {
        equation_space(parser);
        char c = parser->input[parser->position];
        if (isdigit((unsigned char)c) || c == '.'
            || (parser->integer && parser->base == 16 && isxdigit((unsigned char)c)))
          {
             if (parser->integer)
               {
                  unsigned base = parser->base;
                  if (equation_take(parser, "0x") || equation_take(parser, "0X"))
                    base = 16;
                  else if (equation_take(parser, "0b") || equation_take(parser, "0B"))
                    base = 2;
                  else if (equation_take(parser, "0d") || equation_take(parser, "0D"))
                    base = 10;
                  size_t start = parser->position;
                  for (;;)
                    {
                       c = parser->input[parser->position];
                       unsigned digit = c >= '0' && c <= '9'   ? (unsigned)(c - '0')
                                        : c >= 'a' && c <= 'f' ? (unsigned)(c - 'a' + 10)
                                        : c >= 'A' && c <= 'F' ? (unsigned)(c - 'A' + 10)
                                                               : 99;
                       if (digit >= base) break;
                       if (value.bits > (UINT64_MAX - digit) / base)
                         {
                            equation_fail(parser, _("Integer literal exceeds 64 bits"));
                            break;
                         }
                       value.bits = value.bits * base + digit;
                       parser->position++;
                    }
                  if (parser->position == start) equation_fail(parser, _("Expected integer digits"));
               }
             else
               {
                  char *end;
                  const char *start = parser->input + parser->position;
                  value.real = strtold(start, &end);
                  if (end == start) equation_fail(parser, _("Invalid number"));
                  parser->position = (size_t)(end - parser->input);
               }
          }
        else
          {
             char name[32];
             size_t start = parser->position;
             while (isalpha((unsigned char)parser->input[parser->position]))
               parser->position++;
             size_t length = parser->position - start;
             if (!length || length >= sizeof(name) || parser->integer)
               equation_fail(parser, _("Unknown number or function"));
             else
               {
                  memcpy(name, parser->input + start, length);
                  name[length] = 0;
                  const long double pi = acosl(-1.0L);
                  if (!strcmp(name, "pi"))
                    value.real = pi;
                  else if (!strcmp(name, "e"))
                    value.real = expl(1.0L);
                  else if (!equation_take(parser, "("))
                    equation_fail(parser, _("Expected function argument in parentheses"));
                  else
                    {
                       Equation_Value argument = equation_expression(parser, 0);
                       if (!equation_take(parser, ")"))
                         equation_fail(parser, _("Expected closing parenthesis"));
                       long double arg = argument.real;
                       long double angle = parser->degrees ? arg * pi / 180 : arg;
                       if (!strcmp(name, "sin"))
                         value.real = sinl(angle);
                       else if (!strcmp(name, "cos"))
                         value.real = cosl(angle);
                       else if (!strcmp(name, "tan"))
                         value.real = tanl(angle);
                       else if (!strcmp(name, "asin"))
                         value.real = asinl(arg) * (parser->degrees ? 180 / pi : 1);
                       else if (!strcmp(name, "acos"))
                         value.real = acosl(arg) * (parser->degrees ? 180 / pi : 1);
                       else if (!strcmp(name, "atan"))
                         value.real = atanl(arg) * (parser->degrees ? 180 / pi : 1);
                       else if (!strcmp(name, "sqrt"))
                         value.real = sqrtl(arg);
                       else if (!strcmp(name, "ln"))
                         value.real = logl(arg);
                       else if (!strcmp(name, "log"))
                         value.real = log10l(arg);
                       else if (!strcmp(name, "exp"))
                         value.real = expl(arg);
                       else if (!strcmp(name, "floor"))
                         value.real = floorl(arg);
                       else if (!strcmp(name, "ceil"))
                         value.real = ceill(arg);
                       else if (!strcmp(name, "abs"))
                         value.real = fabsl(arg);
                       else
                         equation_fail(parser, _("Unknown function"));
                    }
               }
          }
     }
   if (!parser->integer && !isfinite(value.real))
     equation_fail(parser, _("Result is outside the real finite number range"));
   parser->depth--;
   return value;
}

static Equation_Value
equation_expression(Equation_Parser *parser, int minimum)
{
   Equation_Value left = equation_primary(parser);
   while (!parser->result->error[0])
     {
        equation_space(parser);
        const char *op = NULL;
        static const char *const operators[] = { "<<", ">>", "|", "^", "&", "+", "-", "*", "/", "%" };
        for (size_t i = 0; i < sizeof(operators) / sizeof(operators[0]); i++)
          {
             if (!strncmp(parser->input + parser->position, operators[i], strlen(operators[i])))
               {
                  op = operators[i];
                  break;
               }
          }
        if (!op) break;
        int precedence = *op == '|'                   ? 1
                         : *op == '^'                 ? (parser->integer ? 2 : 7)
                         : *op == '&'                 ? 3
                         : (*op == '<' || *op == '>') ? 4
                         : (*op == '+' || *op == '-') ? 5
                                                      : 6;
        if (precedence < minimum) break;
        parser->position += strlen(op);
        if (++parser->depth > 128)
          {
             equation_fail(parser, _("Expression is too deeply nested"));
             parser->depth--;
             break;
          }
        Equation_Value right
          = equation_expression(parser, precedence + ((!parser->integer && *op == '^') ? 0 : 1));
        parser->depth--;
        if (parser->result->error[0]) break;
        if (parser->integer)
          {
             switch (*op)
               {
                case '+':
                  left.bits += right.bits;
                  break;
                case '-':
                  left.bits -= right.bits;
                  break;
                case '*':
                  left.bits *= right.bits;
                  break;
                case '/':
                case '%':
                  if (!right.bits)
                    equation_fail(parser, _("Division by zero"));
                  else if (*op == '/')
                    left.bits /= right.bits;
                  else
                    left.bits %= right.bits;
                  break;
                case '^':
                  left.bits ^= right.bits;
                  break;
                case '&':
                  left.bits &= right.bits;
                  break;
                case '|':
                  left.bits |= right.bits;
                  break;
                default:
                  if (right.bits >= 64)
                    equation_fail(parser, _("Shift count must be between 0 and 63"));
                  else if (*op == '<')
                    left.bits <<= right.bits;
                  else
                    left.bits >>= right.bits;
               }
          }
        else
          {
             switch (*op)
               {
                case '+':
                  left.real += right.real;
                  break;
                case '-':
                  left.real -= right.real;
                  break;
                case '*':
                  left.real *= right.real;
                  break;
                case '/':
                case '%':
                  if (right.real == 0)
                    equation_fail(parser, _("Division by zero"));
                  else if (*op == '/')
                    left.real /= right.real;
                  else
                    left.real = fmodl(left.real, right.real);
                  break;
                case '^':
                  left.real = powl(left.real, right.real);
                  break;
                default:
                  equation_fail(parser, _("Bitwise operators require programmer mode"));
               }
             if (!isfinite(left.real))
               equation_fail(parser, _("Result is outside the real finite number range"));
          }
     }
   return left;
}

Equation_Result
equation_evaluate_base(const char *expression, Equation_Mode mode, bool degrees, unsigned base)
{
   Equation_Result result = { 0 };
   if (base != 2 && base != 10 && base != 16)
     {
        snprintf(result.error, sizeof(result.error), "%s", _("Unsupported input base"));
        return result;
     }
   if (!expression || strlen(expression) > 4096)
     {
        snprintf(result.error, sizeof(result.error), "%s", _("Expression is missing or too long"));
        return result;
     }
   Equation_Parser parser = { .input = expression,
                               .integer = mode == EQUATION_MODE_PROGRAMMER,
                               .degrees = degrees,
                               .base = base,
                               .result = &result };
   Equation_Value value = equation_expression(&parser, 0);
   equation_space(&parser);
   if (expression[parser.position]) equation_fail(&parser, _("Unexpected input"));
   if (!parser.integer && !isfinite(value.real))
     equation_fail(&parser, _("Result is outside the real finite number range"));
   if (result.error[0]) return result;
   uint64_t bits;
   bool negative = false;
   if (parser.integer)
     {
        bits = value.bits;
        snprintf(result.decimal, sizeof(result.decimal), "%" PRIu64, bits);
     }
   else
     {
        snprintf(result.decimal, sizeof(result.decimal), "%.*Lg", LDBL_DIG, value.real);
        negative = value.real < 0;
        long double magnitude = fabsl(value.real);
        if (truncl(value.real) != value.real || magnitude >= ldexpl(1.0L, 64))
          {
             snprintf(result.hexadecimal, sizeof(result.hexadecimal), "%s",
                      _("Requires an integer within 64 bits"));
             snprintf(result.binary, sizeof(result.binary), "%s",
                      _("Requires an integer within 64 bits"));
             result.valid = true;
             return result;
          }
        bits = (uint64_t)magnitude;
     }
   snprintf(result.hexadecimal, sizeof(result.hexadecimal), "%s0x%" PRIX64, negative ? "-" : "",
            bits);
   size_t offset = (size_t)snprintf(result.binary, sizeof(result.binary), "%s", negative ? "-" : "");
   bool started = false;
   for (int bit = 63; bit >= 0; bit--)
     {
        bool set = ((bits >> bit) & 1) != 0;
        if (set || started || bit == 0)
          {
             result.binary[offset++] = set ? '1' : '0';
             started = true;
          }
     }
   result.binary[offset] = 0;
   result.valid = true;
   return result;
}

Equation_Result
equation_evaluate(const char *expression, Equation_Mode mode, bool degrees)
{
   return equation_evaluate_base(expression, mode, degrees, 10);
}
