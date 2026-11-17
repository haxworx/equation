/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) Enlightenment Development Team
 */

#include <Elementary.h>
#include "equation_ui.h"
#include "equation_intl.h"
int
main(int argc, char **argv)
{
   if (elm_init(argc, argv) <= 0) return 1;
   equation_intl_init();
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   if (!equation_ui_show())
     {
        elm_shutdown();
        return 1;
     }
   elm_run();
   elm_shutdown();
   return 0;
}
