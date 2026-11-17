/* SPDX-License-Identifier: GPL-2.0-only */
#include "equation_intl.h"
#include "equation_engine.h"
#include <locale.h>
#include <stdio.h>
#include <string.h>

int
main(int argc, char **argv)
{
#if EQUATION_ENABLE_NLS
   if (argc != 2) return 1;
   if (!setlocale(LC_ALL, "")) return 77;
   equation_intl_init();
   bindtextdomain("equation", argv[1]);
   if (strcmp(_("Clear"), "Effacer")) return 1;
   if (strcmp(localeconv()->decimal_point, ".")) return 1;
   Equation_Result result = equation_evaluate("1.5+2.25", EQUATION_MODE_SCIENTIFIC, false);
   if (!result.valid || strcmp(result.decimal, "3.75")) return 1;
   result = equation_evaluate("1/0", EQUATION_MODE_SCIENTIFIC, false);
   if (result.valid || !strstr(result.error, "Division par zéro (colonne"))
     {
        fprintf(stderr, "Untranslated diagnostic: %s\n", result.error);
        return 1;
     }
#else
   (void)argc;
   (void)argv;
#endif
   return 0;
}
