/* SPDX-License-Identifier: GPL-2.0-only */
#include "equation_intl.h"
#include <locale.h>

void
equation_intl_init(void)
{
   setlocale(LC_ALL, "");
   /* Expressions, literals and copied results always use a decimal point. */
   setlocale(LC_NUMERIC, "C");
#if EQUATION_ENABLE_NLS
   bindtextdomain("equation", EQUATION_LOCALEDIR);
   bind_textdomain_codeset("equation", "UTF-8");
   textdomain("equation");
#endif
}
